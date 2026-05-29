// SensorHardware.cpp
#include "sensorhardware.h"
#include <QStringList>
#include <QThread>
#include <QDateTime>
#include <QtConcurrent>

SensorHardware::SensorHardware(const QString& portName, QObject *parent)
    : QObject(parent), m_portName(portName)
{
    qDebug() << "SensorHardware создан для порта:" << portName;
}

SensorHardware::~SensorHardware()
{
    stop();
}

bool SensorHardware::start()
{
    if (m_running) return true;
    if (!openSerial()) return false;

    m_running = true;
    QtConcurrent::run([this]() { readLoop(); });

    qDebug() << "Поток чтения запущен для" << m_portName;
    return true;
}

void SensorHardware::stop()
{
    m_running = false;
    closeSerial();
}

double SensorHardware::getWaterWeight() const
{
    QMutexLocker lock(&m_dataMutex);
    return m_waterWeight;
}

double SensorHardware::getFoodWeight() const
{
    QMutexLocker lock(&m_dataMutex);
    return m_foodWeight;
}

double SensorHardware::getTotalWeight() const
{
    QMutexLocker lock(&m_dataMutex);
    return m_totalWeight;
}

void SensorHardware::sendCommand(const QString& cmd)
{
    if (m_hSerial == INVALID_HANDLE_VALUE) {
        qDebug() << "Невозможно отправить команду — порт не открыт";
        return;
    }

    QByteArray data = (cmd + "\n").toUtf8();
    DWORD written = 0;
    WriteFile(m_hSerial, data.constData(), data.size(), &written, nullptr);

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    emit rawDataReceived(QString("[%1] TX: %2").arg(timestamp, cmd));
    qDebug() << "Отправлена команда:" << cmd;
}

bool SensorHardware::openSerial()
{
    QString fullPath = "\\\\.\\" + m_portName;

    qDebug() << "Открытие порта:" << fullPath;

    m_hSerial = CreateFileA(
        fullPath.toStdString().c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr
    );

    if (m_hSerial == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        qDebug() << "Ошибка открытия порта! Код:" << error;
        emit errorOccurred(QString("Не удалось открыть порт %1 (ошибка %2)").arg(m_portName).arg(error));
        return false;
    }

    qDebug() << "Порт открыт успешно. Настройка параметров...";

    DCB dcb = { sizeof(DCB) };
    if (!GetCommState(m_hSerial, &dcb)) {
        qDebug() << "Ошибка GetCommState";
        CloseHandle(m_hSerial);
        m_hSerial = INVALID_HANDLE_VALUE;
        return false;
    }

    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    if (!SetCommState(m_hSerial, &dcb)) {
        qDebug() << "Ошибка SetCommState";
        CloseHandle(m_hSerial);
        m_hSerial = INVALID_HANDLE_VALUE;
        return false;
    }

    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout         = 50;
    timeouts.ReadTotalTimeoutConstant    = 100;
    timeouts.ReadTotalTimeoutMultiplier  = 10;
    SetCommTimeouts(m_hSerial, &timeouts);

    m_connected = true;
    qDebug() << "Порт настроен: 9600 бод, 8N1";
    emit connectionStatusChanged(true, "Подключено к " + m_portName);
    emit rawDataReceived(QString("=== Подключено к %1 ===").arg(m_portName));
    return true;
}

void SensorHardware::closeSerial()
{
    if (m_hSerial != INVALID_HANDLE_VALUE) {
        qDebug() << "Закрытие порта";
        CloseHandle(m_hSerial);
        m_hSerial = INVALID_HANDLE_VALUE;
    }
    m_connected = false;
    emit connectionStatusChanged(false, "Отключено");
    emit rawDataReceived("=== Отключено ===");
}

void SensorHardware::readLoop()
{
    qDebug() << "readLoop запущен. Ожидание данных...";

    char buffer[256];
    QByteArray lineBuffer;

    while (m_running) {
        DWORD bytesRead = 0;
        BOOL result = ReadFile(m_hSerial, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);

        if (!result) {
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING) {
                qDebug() << "Ошибка чтения из порта! Код:" << error;
            }
        }

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            lineBuffer.append(buffer);

            qDebug() << "Прочитано" << bytesRead << "байт:" << QByteArray(buffer, bytesRead).toHex();

            int pos;
            while ((pos = lineBuffer.indexOf('\n')) != -1) {
                QByteArray line = lineBuffer.left(pos).trimmed();
                lineBuffer.remove(0, pos + 1);

                if (!line.isEmpty()) {
                    QString lineStr = QString::fromUtf8(line);

                    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
                    emit rawDataReceived(QString("[%1] RX: %2").arg(timestamp, lineStr));

                    qDebug() << "Получена строка:" << lineStr;
                    parseLine(lineStr);
                }
            }
        }
        QThread::msleep(10);
    }

    qDebug() << "readLoop завершён";
}

void SensorHardware::parseLine(const QString& line)
{
    qDebug() << "parseLine:" << line;

    if (!line.startsWith("DATA:")) {
        qDebug() << "  -> не DATA, пропускаем";
        return;
    }

    QString data = line.mid(5);
    QStringList parts = data.split(',');

    qDebug() << "  -> parts:" << parts;

    if (parts.size() >= 2) {
        bool ok1, ok2, ok3 = false;
        double w = parts[0].toDouble(&ok1);
        double f = parts[1].toDouble(&ok2);
        double t = (parts.size() >= 3) ? parts[2].toDouble(&ok3) : (w + f);

        qDebug() << "  -> w=" << w << "f=" << f << "t=" << t << "ok1=" << ok1 << "ok2=" << ok2;

        if (ok1 && ok2) {
            QMutexLocker lock(&m_dataMutex);
            m_waterWeight = w;
            m_foodWeight  = f;
            m_totalWeight = ok3 ? t : (w + f);
            m_hasData = true;

            qDebug() << "  ✅ Данные обновлены!";
            emit dataReceived(m_waterWeight, m_foodWeight, m_totalWeight);
        }
    }
}

// SensorHardware.h
#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QDebug>
#include <windows.h>
#include <atomic>

class SensorHardware : public QObject
{
    Q_OBJECT

public:
    explicit SensorHardware(const QString& portName, QObject *parent = nullptr);
    ~SensorHardware() override;

    bool start();
    void stop();

    bool isConnected() const { return m_connected; }
    QString portName() const { return m_portName; }

    double getWaterWeight() const;
    double getFoodWeight() const;
    double getTotalWeight() const;
    bool hasValidData() const { return m_hasData; }

    void sendCommand(const QString& cmd);

signals:
    void dataReceived(double water_g, double food_g, double total_g);
    void connectionStatusChanged(bool connected, const QString& status);
    void errorOccurred(const QString& error);
    void rawDataReceived(const QString& rawLine);  // ⭐ НОВЫЙ СИГНАЛ для лога

private:
    void readLoop();
    void parseLine(const QString& line);
    bool openSerial();
    void closeSerial();

    QString m_portName;
    HANDLE m_hSerial = INVALID_HANDLE_VALUE;
    std::atomic<bool> m_running{ false };
    std::atomic<bool> m_connected{ false };
    std::atomic<bool> m_hasData{ false };

    mutable QMutex m_dataMutex;
    double m_waterWeight = 0.0;
    double m_foodWeight  = 0.0;
    double m_totalWeight = 0.0;
};

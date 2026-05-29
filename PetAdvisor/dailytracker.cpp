// DailyTracker.cpp
#include "dailytracker.h"
#include <algorithm>

DailyTracker::DailyTracker(QObject *parent)
    : QObject(parent)
{
    m_saveTimer = new QTimer(this);
    m_saveTimer->setTimerType(Qt::VeryCoarseTimer);  // для интервалов > 1 сек
    connect(m_saveTimer, &QTimer::timeout, this, &DailyTracker::onSaveTimer);

    qDebug() << "DailyTracker создан. Запуск таймера с интервалом:" << m_saveInterval << "сек";
}

void DailyTracker::setSaveInterval(int seconds)
{
    m_saveInterval = seconds;

    // Перезапускаем таймер с новым интервалом
    if (m_saveTimer->isActive()) {
        m_saveTimer->stop();
    }
    m_saveTimer->start(m_saveInterval * 1000);

    qDebug() << "Интервал сохранения изменён на" << seconds << "сек";
}

void DailyTracker::startTracking()
{
    if (!m_saveTimer->isActive()) {
        m_saveTimer->start(m_saveInterval * 1000);
        qDebug() << "Таймер запущен. Интервал:" << m_saveInterval << "сек";
    }
}

void DailyTracker::stopTracking()
{
    if (m_saveTimer->isActive()) {
        m_saveTimer->stop();
        qDebug() << "Таймер остановлен";
    }
}

void DailyTracker::updateCurrentWeight(double water_ml, double food_g)
{
    m_currentWater = water_ml;
    m_currentFood  = food_g;

    // При первом получении данных фиксируем начальные веса и запускаем таймер
    if (!m_initialized) {
        m_startWater = water_ml;
        m_startFood  = food_g;
        m_initialized = true;

        // Запускаем таймер при первом получении данных
        if (!m_saveTimer->isActive()) {
            m_saveTimer->start(m_saveInterval * 1000);
            qDebug() << "🟢 Трекер инициализирован и таймер запущен. Вода:" << m_startWater << "г, Корм:" << m_startFood << "г";
        }

        emit consumptionUpdated(0, 0);
        return;
    }

    // Вычисляем потребление с момента последнего сохранения
    double waterConsumed = m_startWater - m_currentWater;
    double foodConsumed  = m_startFood  - m_currentFood;

    // Защита от отрицательных значений (если долили/досыпали)
    if (waterConsumed < 0) {
        m_startWater = m_currentWater;
        waterConsumed = 0;
        qDebug() << "💧 Обнаружено пополнение воды. Сброс начального веса.";
    }
    if (foodConsumed < 0) {
        m_startFood = m_currentFood;
        foodConsumed = 0;
        qDebug() << "🍖 Обнаружено пополнение корма. Сброс начального веса.";
    }

    m_consumedWater = waterConsumed;
    m_consumedFood  = foodConsumed;

    qDebug() << "📊 Текущее потребление: Корм" << m_consumedFood << "г, Вода" << m_consumedWater << "мл";
    emit consumptionUpdated(m_consumedFood, m_consumedWater);
}

void DailyTracker::onSaveTimer()
{
    qDebug() << "⏰ ТАЙМЕР СРАБОТАЛ! initialized =" << m_initialized
             << "consumedFood =" << m_consumedFood << "consumedWater =" << m_consumedWater;

    if (!m_initialized) {
        qDebug() << "❌ Трекер не инициализирован — пропускаем сохранение";
        return;
    }

    // Сохраняем, даже если потребление = 0 (чтобы было видно в таблице)
    saveCurrentRecord();

    // Сбрасываем начальные веса на текущие для следующего интервала
    m_startWater = m_currentWater;
    m_startFood  = m_currentFood;
    m_consumedWater = 0;
    m_consumedFood  = 0;

    qDebug() << "🔄 Начальные веса сброшены. Таймер перезапущен.";
}

void DailyTracker::forceSave()
{
    qDebug() << "⚡ Принудительное сохранение";
    if (!m_initialized) {
        qDebug() << "❌ Трекер не инициализирован";
        return;
    }
    saveCurrentRecord();

    // Сбрасываем начальные веса
    m_startWater = m_currentWater;
    m_startFood  = m_currentFood;
    m_consumedWater = 0;
    m_consumedFood  = 0;
}

void DailyTracker::saveCurrentRecord()
{
    Record record;
    record.timestamp = QDateTime::currentDateTime();
    record.foodConsumed  = m_consumedFood;
    record.waterConsumed = m_consumedWater;

    qDebug() << "💾 СОХРАНЕНИЕ ЗАПИСИ:"
             << record.timestamp.toString("hh:mm:ss")
             << "| Корм:" << record.foodConsumed << "г"
             << "| Вода:" << record.waterConsumed << "мл";

    updateHistory(record);

    emit recordSaved(record.timestamp, record.foodConsumed, record.waterConsumed);
}

void DailyTracker::addManualRecord(const QDateTime& timestamp, double food, double water)
{
    Record record;
    record.timestamp = timestamp;
    record.foodConsumed  = food;
    record.waterConsumed = water;
    updateHistory(record);
}

void DailyTracker::updateHistory(const Record& record)
{
    m_history.append(record);

    std::sort(m_history.begin(), m_history.end(),
        [](const Record& a, const Record& b) { return a.timestamp < b.timestamp; });

    while (m_history.size() > 5) {
        m_history.removeFirst();
    }

    qDebug() << "📋 История обновлена. Всего записей:" << m_history.size();

    for (int i = 0; i < m_history.size(); ++i) {
        qDebug() << "  [" << i << "]" << m_history[i].timestamp.toString("hh:mm:ss")
                 << "Корм:" << m_history[i].foodConsumed << "Вода:" << m_history[i].waterConsumed;
    }

    emit historyUpdated(m_history);
}

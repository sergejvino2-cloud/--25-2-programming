// DailyTracker.h
#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QVector>
#include <QDebug>

class DailyTracker : public QObject
{
    Q_OBJECT

public:
    explicit DailyTracker(QObject *parent = nullptr);

    void setSaveInterval(int seconds);
    void startTracking();
    void stopTracking();

    double getStartWater() const { return m_startWater; }
    double getStartFood()  const { return m_startFood; }

    double getConsumedWater() const { return m_consumedWater; }
    double getConsumedFood()  const { return m_consumedFood; }

    struct Record {
        QDateTime timestamp;
        double foodConsumed;
        double waterConsumed;
    };

    QVector<Record> getLast5Records() const { return m_history; }

    void addManualRecord(const QDateTime& timestamp, double food, double water);

public slots:
    void updateCurrentWeight(double water_ml, double food_g);
    void forceSave();

signals:
    void recordSaved(const QDateTime& timestamp, double foodConsumed, double waterConsumed);
    void historyUpdated(const QVector<DailyTracker::Record>& records);
    void consumptionUpdated(double food_g, double water_ml);

private slots:
    void onSaveTimer();

private:
    QTimer* m_saveTimer = nullptr;
    int m_saveInterval = 30;  // секунды

    double m_startWater = 0.0;
    double m_startFood  = 0.0;

    double m_currentWater = 0.0;
    double m_currentFood  = 0.0;

    double m_consumedWater = 0.0;
    double m_consumedFood  = 0.0;

    bool m_initialized = false;

    QVector<Record> m_history;

    void saveCurrentRecord();
    void updateHistory(const Record& record);
};

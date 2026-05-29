// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QVector>
#include <QTimer>
#include "sensorhardware.h"
#include "dailytracker.h"

struct BreedNorm {
    QString nameRus;
    double avgFood;
    double avgWater;
    double weightMin;
    double weightMax;
    double heightMin;
    double heightMax;
};

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSpeciesChanged(int index);
    void onBreedChanged(int index);
    void onAnalyzeClicked();

    void onConnectSensor();
    void onDisconnectSensor();
    void onSensorDataReceived(double water, double food, double total);
    void onConnectionStatus(bool connected, const QString& status);
    void onSensorError(const QString& error);
    void onRawDataReceived(const QString& rawLine);

    void onRecordSaved(const QDateTime& timestamp, double food, double water);  // ⭐ ИЗМЕНЕНО
    void onHistoryUpdated(const QVector<DailyTracker::Record>& records);        // ⭐ ИЗМЕНЕНО
    void onConsumptionUpdated(double food, double water);

    void onAutoAnalyzeCheck();
    void onTableCellChanged(int row, int column);

private:
    Ui::MainWindow *ui;

    QMap<QString, QMap<QString, BreedNorm>> speciesBreeds;
    QSet<QString> seriousDiseases;
    QSet<QString> minorDiseases;

    SensorHardware* m_sensor = nullptr;
    DailyTracker* m_tracker = nullptr;
    QTimer* m_autoAnalyzeTimer = nullptr;

    bool m_sensorEnabled = false;
    bool m_manualInputMode = false;

    void initData();
    void initSensors();
    void setTableEditMode(bool editable);
    void addLogMessage(const QString& message);

    QStringList breedsForSpecies(const QString &speciesEng) const;
    BreedNorm getBreedNorm(const QString &speciesEng, const QString &breedEng) const;
    QString analyzeFiveDays(const QString &speciesEng, const QString &breedEng,
                            double weight, double age,
                            const QStringList &diseases,
                            const QVector<double> &foods,
                            const QVector<double> &waters) const;

    void drawGraphs(const QVector<double> &foods, const QVector<double> &waters,
                    double foodNorm, double waterNorm);

    void showAnalysisDialog(const QString& species, const QString& breed,
                           double weight, double age,
                           const QStringList& diseases,
                           const QVector<double>& foods,
                           const QVector<double>& waters);
};

#endif

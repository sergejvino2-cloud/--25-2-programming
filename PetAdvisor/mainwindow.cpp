// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "analysisdialog.h"
#include <QMessageBox>
#include <QListWidgetItem>
#include <QPainter>
#include <QPixmap>
#include <QDebug>
#include <algorithm>
#include <QInputDialog>
#include <QTime>
#include <QDateTime>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initData();
    initSensors();

    ui->speciesCombo->addItems({"dog", "cat", "rabbit"});

    connect(ui->speciesCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSpeciesChanged);
    connect(ui->breedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onBreedChanged);
    connect(ui->analyzeBtn, &QPushButton::clicked,
            this, &MainWindow::onAnalyzeClicked);

    // Настройка таблицы 5 дней
    ui->tableWidget->setRowCount(5);
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setHorizontalHeaderLabels({"Корм (г)", "Вода (мл)"});
    for (int row = 0; row < 5; ++row) {
        QTableWidgetItem *itemFood = new QTableWidgetItem("—");
        QTableWidgetItem *itemWater = new QTableWidgetItem("—");
        itemFood->setTextAlignment(Qt::AlignCenter);
        itemWater->setTextAlignment(Qt::AlignCenter);
        itemFood->setFlags(itemFood->flags() & ~Qt::ItemIsEditable);
        itemWater->setFlags(itemWater->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(row, 0, itemFood);
        ui->tableWidget->setItem(row, 1, itemWater);
    }
    ui->tableWidget->setColumnWidth(0, 100);
    ui->tableWidget->setColumnWidth(1, 100);

    connect(ui->tableWidget, &QTableWidget::cellChanged,
            this, &MainWindow::onTableCellChanged);

    // Заболевания
    QMap<QString, QString> diseaseMap;
    diseaseMap["allergy"] = "Аллергия";
    diseaseMap["arthritis"] = "Артрит";
    diseaseMap["diabetes"] = "Диабет";
    diseaseMap["kidney_disease"] = "Болезнь почек";
    diseaseMap["obesity"] = "Ожирение";
    diseaseMap["underweight"] = "Недостаточный вес";

    for (auto it = diseaseMap.begin(); it != diseaseMap.end(); ++it) {
        QListWidgetItem *item = new QListWidgetItem(it.value());
        item->setData(Qt::UserRole, it.key());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        ui->diseaseList->addItem(item);
    }
    ui->diseaseList->setToolTip("Отметьте все имеющиеся заболевания");

    // Кнопки датчиков
    if (ui->connectSensorBtn) {
        connect(ui->connectSensorBtn, &QPushButton::clicked,
                this, &MainWindow::onConnectSensor);
    }
    if (ui->disconnectSensorBtn) {
        connect(ui->disconnectSensorBtn, &QPushButton::clicked,
                this, &MainWindow::onDisconnectSensor);
    }

    // Настройка лога COM-порта
    ui->comLogTextEdit->setReadOnly(true);
    ui->comLogTextEdit->setStyleSheet(
        "QTextEdit {"
        "  background-color: #1e1e1e;"
        "  color: #00ff00;"
        "  font-family: 'Courier New', monospace;"
        "  font-size: 11px;"
        "  border: 1px solid #555;"
        "  border-radius: 3px;"
        "  padding: 5px;"
        "}"
    );

    setTableEditMode(true);
    ui->sensorStatusLabel->setText("Статус: Датчики не подключены (ручной ввод)");

    addLogMessage("Приложение запущено. Ожидание подключения датчиков...");
}

MainWindow::~MainWindow()
{
    if (m_sensor) {
        m_sensor->stop();
    }
    delete ui;
}

// ==================== ЛОГ CO M-ПОРТА ====================
void MainWindow::addLogMessage(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp, message);

    ui->comLogTextEdit->append(logEntry);

    QScrollBar* sb = ui->comLogTextEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void MainWindow::onRawDataReceived(const QString& rawLine)
{
    ui->comLogTextEdit->append(rawLine);

    QScrollBar* sb = ui->comLogTextEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}

// ==================== ПЕРЕКЛЮЧЕНИЕ РЕЖИМА ТАБЛИЦЫ ====================
void MainWindow::setTableEditMode(bool editable)
{
    m_manualInputMode = editable;

    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        for (int col = 0; col < ui->tableWidget->columnCount(); ++col) {
            QTableWidgetItem* item = ui->tableWidget->item(row, col);
            if (!item) continue;

            if (editable) {
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                item->setForeground(Qt::black);
            } else {
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                item->setForeground(QColor(0, 100, 0));
            }
        }
    }

    qDebug() << "Режим таблицы:" << (editable ? "Ручной ввод" : "Данные с датчиков");
}

// ==================== РУЧНОЕ ИЗМЕНЕНИЕ ЯЧЕЙКИ ====================
void MainWindow::onTableCellChanged(int row, int column)
{
    if (!m_manualInputMode) return;

    QTableWidgetItem* item = ui->tableWidget->item(row, column);
    if (!item) return;

    QString text = item->text();
    bool ok;
    double value = text.toDouble(&ok);

    if (!ok || value < 0) {
        if (text != "—" && !text.isEmpty()) {
            QMessageBox::warning(this, "Ошибка ввода",
                                 "Введите положительное число.\n"
                                 "Для пропуска оставьте \"—\".");
            item->setText("0");
        }
        return;
    }

    qDebug() << "Ручной ввод: строка" << row << ", столбец" << column << "=" << value;

    bool allFilled = true;
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        for (int c = 0; c < ui->tableWidget->columnCount(); ++c) {
            QTableWidgetItem* it = ui->tableWidget->item(r, c);
            if (!it || it->text() == "—" || it->text().isEmpty()) {
                allFilled = false;
                break;
            }
        }
        if (!allFilled) break;
    }
}

// ==================== ИНИЦИАЛИЗАЦИЯ ДАННЫХ ====================
void MainWindow::initData()
{
    QMap<QString, BreedNorm> dogBreeds;
    dogBreeds["labrador"]        = {"Лабрадор", 400, 650, 25, 36, 55, 62};
    dogBreeds["german_shepherd"] = {"Немецкая овчарка", 450, 750, 30, 40, 55, 65};
    dogBreeds["bulldog"]         = {"Бульдог", 320, 500, 20, 25, 30, 40};
    dogBreeds["poodle"]          = {"Пудель", 250, 450, 8, 25, 25, 60};
    dogBreeds["chihuahua"]       = {"Чихуахуа", 75, 150, 1, 3, 15, 25};
    dogBreeds["husky"]           = {"Хаски", 400, 650, 20, 27, 50, 60};
    dogBreeds["dachshund"]       = {"Такса", 220, 350, 7, 14, 20, 27};
    dogBreeds["rottweiler"]      = {"Ротвейлер", 500, 850, 38, 50, 56, 69};
    dogBreeds["beagle"]          = {"Бигль", 240, 400, 9, 11, 33, 41};
    dogBreeds["boxer"]           = {"Боксёр", 420, 680, 25, 32, 53, 63};
    dogBreeds["mongrel"]         = {"Беспородная собака", 350, 550, 10, 35, 25, 60};
    speciesBreeds["dog"] = dogBreeds;

    QMap<QString, BreedNorm> catBreeds;
    catBreeds["persian"]          = {"Персидская", 60, 200, 3.5, 7, 20, 30};
    catBreeds["siamese"]          = {"Сиамская", 50, 160, 3, 5, 25, 35};
    catBreeds["maine_coon"]       = {"Мейн-кун", 90, 280, 5, 10, 30, 40};
    catBreeds["sphynx"]           = {"Сфинкс", 70, 200, 3, 5, 20, 30};
    catBreeds["british_shorthair"]= {"Британская", 75, 230, 4, 8, 25, 35};
    catBreeds["bengal"]           = {"Бенгальская", 78, 230, 3.5, 6.5, 25, 35};
    catBreeds["ragdoll"]          = {"Рэгдолл", 85, 250, 4.5, 9, 28, 38};
    catBreeds["scottish_fold"]    = {"Шотландская вислоухая", 60, 200, 3, 6, 20, 30};
    catBreeds["abyssinian"]       = {"Абиссинская", 55, 180, 3, 5, 25, 35};
    catBreeds["mongrel"]          = {"Беспородная кошка", 65, 210, 2.5, 7, 20, 35};
    speciesBreeds["cat"] = catBreeds;

    QMap<QString, BreedNorm> rabbitBreeds;
    rabbitBreeds["dwarf"]         = {"Карликовый", 30, 75, 0.5, 1.5, 8, 15};
    rabbitBreeds["lop"]           = {"Вислоухий", 55, 140, 1.5, 3.0, 15, 25};
    rabbitBreeds["angora"]        = {"Ангорский", 65, 160, 2.0, 3.5, 18, 28};
    rabbitBreeds["rex"]           = {"Рекс", 58, 140, 1.8, 3.2, 15, 25};
    rabbitBreeds["flemish_giant"] = {"Фландр", 150, 300, 5.0, 10.0, 30, 50};
    rabbitBreeds["dutch"]         = {"Датский", 50, 125, 1.5, 2.5, 12, 22};
    rabbitBreeds["lionhead"]      = {"Львиноголовый", 42, 100, 1.2, 1.8, 10, 20};
    rabbitBreeds["mini_rex"]      = {"Мини-рекс", 48, 115, 1.4, 2.0, 12, 20};
    rabbitBreeds["mongrel"]       = {"Беспородный кролик", 50, 120, 1.0, 3.5, 10, 30};
    speciesBreeds["rabbit"] = rabbitBreeds;

    seriousDiseases = {"diabetes", "kidney_disease", "infection"};
    minorDiseases   = {"arthritis", "allergy", "obesity", "underweight"};
}

// ==================== ИНИЦИАЛИЗАЦИЯ ДАТЧИКОВ ====================
void MainWindow::initSensors()
{
    m_tracker = new DailyTracker(this);
    m_tracker->setSaveInterval(30);  // 30 секунд — теперь запускает таймер

    connect(m_tracker, &DailyTracker::recordSaved,
            this, &MainWindow::onRecordSaved);
    connect(m_tracker, &DailyTracker::historyUpdated,
            this, &MainWindow::onHistoryUpdated);
    connect(m_tracker, &DailyTracker::consumptionUpdated,
            this, &MainWindow::onConsumptionUpdated);

    m_autoAnalyzeTimer = new QTimer(this);
    connect(m_autoAnalyzeTimer, &QTimer::timeout,
            this, &MainWindow::onAutoAnalyzeCheck);
    m_autoAnalyzeTimer->start(600000);

    qDebug() << "Система датчиков инициализирована (интервал сохранения: 30 сек)";
    addLogMessage("Трекер инициализирован. Ожидание данных с датчиков...");
}

// ==================== ⭐ СОХРАНЕНИЕ ЗАПИСИ КАЖДЫЕ 30 СЕКУНД ====================
void MainWindow::onRecordSaved(const QDateTime& timestamp, double food, double water)
{
    qDebug() << "=== ЗАПИСЬ СОХРАНЕНА ===";
    qDebug() << "Время:" << timestamp.toString("hh:mm:ss");
    qDebug() << "Потреблено корма:" << food << "г";
    qDebug() << "Потреблено воды:" << water << "мл";

    addLogMessage(QString("💾 Сохранено [%1]: Корм %2 г | Вода %3 мл")
                  .arg(timestamp.toString("hh:mm:ss"))
                  .arg(food, 0, 'f', 1)
                  .arg(water, 0, 'f', 1));

    // Автоматически показываем анализ, если накопилось 5 записей
    auto records = m_tracker->getLast5Records();
    if (records.size() >= 5) {
        QString species = ui->speciesCombo->currentText();
        QString breed = ui->breedCombo->currentText();
        if (!breed.isEmpty()) {
            QVector<double> foods(5), waters(5);
            for (int i = 0; i < 5; ++i) {
                foods[i] = records[i].foodConsumed;
                waters[i] = records[i].waterConsumed;
            }

            double weight = ui->weightSpin->value();
            double age = ui->ageSpin->value();

            QStringList diseases;
            for (int i = 0; i < ui->diseaseList->count(); ++i) {
                QListWidgetItem* item = ui->diseaseList->item(i);
                if (item->checkState() == Qt::Checked) {
                    QString engName = item->data(Qt::UserRole).toString();
                    if (!engName.isEmpty())
                        diseases.append(engName);
                }
            }

            showAnalysisDialog(species, breed, weight, age, diseases, foods, waters);
        }
    }
}

// ==================== СЛОТЫ ВЫБОРА ПОРОДЫ ====================
void MainWindow::onSpeciesChanged(int /*index*/)
{
    QString species = ui->speciesCombo->currentText();
    ui->breedCombo->clear();
    QStringList breeds = breedsForSpecies(species);
    if (!breeds.isEmpty())
        ui->breedCombo->addItems(breeds);
    onBreedChanged(0);
}

void MainWindow::onBreedChanged(int /*index*/)
{
    QString species = ui->speciesCombo->currentText();
    QString breed = ui->breedCombo->currentText();
    if (species.isEmpty() || breed.isEmpty()) return;

    BreedNorm norm = getBreedNorm(species, breed);
    ui->infoLabel->setText(
        QString("<b>%1</b><br>"
                "Корм: %2 г/день, Вода: %3 мл/день<br>"
                "Нормальный вес: %4-%5 кг, Рост: %6-%7 см")
        .arg(norm.nameRus)
        .arg(norm.avgFood)
        .arg(norm.avgWater)
        .arg(norm.weightMin)
        .arg(norm.weightMax)
        .arg(norm.heightMin)
        .arg(norm.heightMax));
}

// ==================== ПОДКЛЮЧЕНИЕ ДАТЧИКОВ ====================
void MainWindow::onConnectSensor()
{
    if (m_sensor && m_sensor->isConnected()) {
        QMessageBox::information(this, "Датчики",
                                 "Уже подключено к " + m_sensor->portName());
        return;
    }

    bool ok;
    QString port = QInputDialog::getText(this, "Подключение датчиков",
                                         "Введите COM-порт (например, COM3):",
                                         QLineEdit::Normal, "COM3", &ok);
    if (!ok || port.isEmpty()) return;

    if (m_sensor) {
        m_sensor->stop();
        delete m_sensor;
    }

    m_sensor = new SensorHardware(port, this);

    connect(m_sensor, &SensorHardware::dataReceived,
            this, &MainWindow::onSensorDataReceived);
    connect(m_sensor, &SensorHardware::connectionStatusChanged,
            this, &MainWindow::onConnectionStatus);
    connect(m_sensor, &SensorHardware::errorOccurred,
            this, &MainWindow::onSensorError);
    connect(m_sensor, &SensorHardware::rawDataReceived,
            this, &MainWindow::onRawDataReceived);

    addLogMessage("Попытка подключения к " + port + "...");

    if (m_sensor->start()) {
        m_sensorEnabled = true;
        setTableEditMode(false);
        ui->sensorStatusLabel->setText("Статус: Подключено к " + port);
        ui->sensorStatusLabel->setStyleSheet("color: green; font-weight: bold;");
        addLogMessage("✅ Успешно подключено к " + port);
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть порт " + port);
        addLogMessage("❌ Ошибка подключения к " + port);
        delete m_sensor;
        m_sensor = nullptr;
    }
}

void MainWindow::onDisconnectSensor()
{
    if (m_sensor) {
        addLogMessage("Отключение от " + m_sensor->portName() + "...");
        m_sensor->stop();
        delete m_sensor;
        m_sensor = nullptr;
    }
    m_sensorEnabled = false;

    setTableEditMode(true);

    ui->sensorStatusLabel->setText("Статус: Датчики не подключены (ручной ввод)");
    ui->sensorStatusLabel->setStyleSheet("color: gray; font-style: italic;");

    addLogMessage("Датчики отключены. Режим ручного ввода.");
    qDebug() << "Датчики отключены. Режим ручного ввода активирован.";
}

// ==================== ДАННЫЕ С ДАТЧИКОВ ====================
void MainWindow::onSensorDataReceived(double water, double food, double total)
{
    m_tracker->updateCurrentWeight(water, food);
    qDebug() << "Датчики: вода =" << water << "г, корм =" << food
             << "г, всего =" << total << "г";
}

void MainWindow::onConnectionStatus(bool connected, const QString& status)
{
    if (connected) {
        ui->sensorStatusLabel->setText("Статус: " + status);
        ui->sensorStatusLabel->setStyleSheet("color: green; font-weight: bold;");
    } else {
        ui->sensorStatusLabel->setText("Статус: " + status);
        ui->sensorStatusLabel->setStyleSheet("color: red; font-style: italic;");
        m_sensorEnabled = false;
        setTableEditMode(true);
    }
}

void MainWindow::onSensorError(const QString& error)
{
    qDebug() << "Ошибка датчиков:" << error;
    addLogMessage("❌ Ошибка: " + error);
}

// ==================== ЗАВЕРШЕНИЕ ДНЯ ====================


// ==================== ОБНОВЛЕНИЕ ТАБЛИЦЫ ====================
void MainWindow::onHistoryUpdated(const QVector<DailyTracker::Record>& records)
{
    for (int row = 0; row < 5; ++row) {
        QTableWidgetItem* foodItem = ui->tableWidget->item(row, 0);
        QTableWidgetItem* waterItem = ui->tableWidget->item(row, 1);

        if (row < records.size()) {
            const auto& rec = records[records.size() - 1 - row];
            foodItem->setText(QString::number(rec.foodConsumed, 'f', 1));
            waterItem->setText(QString::number(rec.waterConsumed, 'f', 1));
            foodItem->setToolTip(rec.timestamp.toString("dd.MM.yyyy hh:mm:ss"));
            waterItem->setToolTip(rec.timestamp.toString("dd.MM.yyyy hh:mm:ss"));
        } else {
            foodItem->setText("—");
            waterItem->setText("—");
            foodItem->setToolTip("");
            waterItem->setToolTip("");
        }
    }
}

// ==================== ТЕКУЩЕЕ ПОТРЕБЛЕНИЕ ====================
void MainWindow::onConsumptionUpdated(double food, double water)
{
    qDebug() << "Сегодня потреблено: корм" << food << "г, вода" << water << "мл";
}

// ==================== АВТО-АНАЛИЗ ====================
void MainWindow::onAutoAnalyzeCheck()
{
    auto records = m_tracker->getLast5Records();

    if (records.size() >= 5) {
        QString species = ui->speciesCombo->currentText();
        QString breed = ui->breedCombo->currentText();
        if (!breed.isEmpty()) {
            QVector<double> foods(5), waters(5);
            for (int i = 0; i < 5; ++i) {
                foods[i] = records[i].foodConsumed;
                waters[i] = records[i].waterConsumed;
            }

            double weight = ui->weightSpin->value();
            double age = ui->ageSpin->value();

            QStringList diseases;
            for (int i = 0; i < ui->diseaseList->count(); ++i) {
                QListWidgetItem* item = ui->diseaseList->item(i);
                if (item->checkState() == Qt::Checked) {
                    QString engName = item->data(Qt::UserRole).toString();
                    if (!engName.isEmpty())
                        diseases.append(engName);
                }
            }

            showAnalysisDialog(species, breed, weight, age, diseases, foods, waters);
        }
    }
}

// ==================== ПОКАЗ ОКНА АНАЛИЗА ====================
void MainWindow::showAnalysisDialog(const QString& species, const QString& breed,
                                     double weight, double age,
                                     const QStringList& diseases,
                                     const QVector<double>& foods,
                                     const QVector<double>& waters)
{
    BreedNorm norm = getBreedNorm(species, breed);

    AnalysisDialog* dialog = new AnalysisDialog(this);

    QString title = QString("<b>%1 (%2)</b><br>"
                           "Вес: %3 кг | Возраст: %4 лет")
                    .arg(norm.nameRus)
                    .arg(species)
                    .arg(weight)
                    .arg(age);
    dialog->setTitle(title);

    QString result = analyzeFiveDays(species, breed, weight, age, diseases, foods, waters);

    QString htmlResult;
    if (result.startsWith("🚨")) {
        htmlResult = "<div style='color: red; font-size: 14px; padding: 10px;'>"
                     + result.replace("\n", "<br>") + "</div>";
    } else if (result.startsWith("⚠")) {
        htmlResult = "<div style='color: orange; font-size: 14px; padding: 10px;'>"
                     + result.replace("\n", "<br>") + "</div>";
    } else if (result.startsWith("📋")) {
        htmlResult = "<div style='color: #2c3e50; font-size: 14px; padding: 10px;'>"
                     + result.replace("\n", "<br>") + "</div>";
    } else if (result.startsWith("✅")) {
        htmlResult = "<div style='color: green; font-size: 14px; padding: 10px;'>"
                     + result.replace("\n", "<br>") + "</div>";
    } else {
        htmlResult = result.replace("\n", "<br>");
    }

    dialog->setResult(htmlResult);
    dialog->setGraphData(foods, waters, norm.avgFood, norm.avgWater);
    dialog->exec();
    delete dialog;

    addLogMessage("Анализ выполнен. Результаты показаны в отдельном окне.");
}

// ==================== КНОПКА "АНАЛИЗ" ====================
void MainWindow::onAnalyzeClicked()
{
    QString species = ui->speciesCombo->currentText();
    QString breed = ui->breedCombo->currentText();
    if (breed.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите породу.");
        return;
    }

    double weight = ui->weightSpin->value();
    double age = ui->ageSpin->value();

    QStringList diseases;
    for (int i = 0; i < ui->diseaseList->count(); ++i) {
        QListWidgetItem *item = ui->diseaseList->item(i);
        if (item->checkState() == Qt::Checked) {
            QString engName = item->data(Qt::UserRole).toString();
            if (!engName.isEmpty())
                diseases.append(engName);
        }
    }

    QVector<double> foods(5), waters(5);

    // Если датчики включены — берём данные из трекера
    if (m_sensorEnabled && m_tracker->getLast5Records().size() >= 5) {
        auto records = m_tracker->getLast5Records();
        for (int i = 0; i < 5; ++i) {
            foods[i] = records[i].foodConsumed;
            waters[i] = records[i].waterConsumed;
        }

        BreedNorm norm = getBreedNorm(species, breed);
        drawGraphs(foods, waters, norm.avgFood, norm.avgWater);
        showAnalysisDialog(species, breed, weight, age, diseases, foods, waters);
        return;
    }

    // Ручной ввод
    for (int row = 0; row < 5; ++row) {
        QTableWidgetItem *foodItem = ui->tableWidget->item(row, 0);
        QTableWidgetItem *waterItem = ui->tableWidget->item(row, 1);
        if (!foodItem || !waterItem) {
            QMessageBox::warning(this, "Ошибка", "Ошибка доступа к ячейкам таблицы.");
            return;
        }

        QString foodText = foodItem->text();
        QString waterText = waterItem->text();

        if (foodText == "—" || foodText.isEmpty() ||
            waterText == "—" || waterText.isEmpty()) {
            QMessageBox::warning(this, "Ошибка",
                                 QString("Заполните все ячейки!\n"
                                         "Строка %1 не заполнена.").arg(row + 1));
            return;
        }

        bool ok1, ok2;
        double f = foodText.toDouble(&ok1);
        double w = waterText.toDouble(&ok2);
        if (!ok1 || !ok2 || f < 0 || w < 0) {
            QMessageBox::warning(this, "Ошибка",
                                 QString("Некорректные числа в строке %1.\n"
                                         "Введите положительные числа.").arg(row + 1));
            return;
        }
        foods[row] = f;
        waters[row] = w;
    }

    BreedNorm norm = getBreedNorm(species, breed);
    drawGraphs(foods, waters, norm.avgFood, norm.avgWater);
    showAnalysisDialog(species, breed, weight, age, diseases, foods, waters);
}

// ==================== ГРАФИКИ ====================
void MainWindow::drawGraphs(const QVector<double> &foods, const QVector<double> &waters,
                            double foodNorm, double waterNorm)
{
    if (ui->foodGraphLabel->width() < 10 || ui->foodGraphLabel->height() < 10) {
        ui->foodGraphLabel->setMinimumSize(250, 180);
    }
    if (ui->waterGraphLabel->width() < 10 || ui->waterGraphLabel->height() < 10) {
        ui->waterGraphLabel->setMinimumSize(250, 180);
    }

    QPixmap foodPix(ui->foodGraphLabel->width(), ui->foodGraphLabel->height());
    QPixmap waterPix(ui->waterGraphLabel->width(), ui->waterGraphLabel->height());
    foodPix.fill(Qt::white);
    waterPix.fill(Qt::white);

    QPainter pFood(&foodPix);
    QPainter pWater(&waterPix);
    pFood.setRenderHint(QPainter::Antialiasing);
    pWater.setRenderHint(QPainter::Antialiasing);

    int margin = 30;
    int wFood = foodPix.width();
    int hFood = foodPix.height();
    int wWater = waterPix.width();
    int hWater = waterPix.height();

    double maxFood = *std::max_element(foods.begin(), foods.end());
    double maxWater = *std::max_element(waters.begin(), waters.end());
    maxFood = qMax(maxFood, foodNorm * 1.5);
    maxWater = qMax(maxWater, waterNorm * 1.5);
    double scaleFood = (hFood - 2 * margin) / maxFood;
    double scaleWater = (hWater - 2 * margin) / maxWater;
    double stepX = (wFood - 2.0 * margin) / 4.0;

    pFood.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    pWater.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    double yNormFood = hFood - margin - foodNorm * scaleFood;
    double yNormWater = hWater - margin - waterNorm * scaleWater;
    pFood.drawLine(margin, yNormFood, wFood - margin, yNormFood);
    pWater.drawLine(margin, yNormWater, wWater - margin, yNormWater);
    pFood.drawText(margin + 5, yNormFood - 5, "Норма");
    pWater.drawText(margin + 5, yNormWater - 5, "Норма");

    pFood.setPen(Qt::black);
    pWater.setPen(Qt::black);
    pFood.drawText(0, hFood/2, "Корм (г)");
    pWater.drawText(0, hWater/2, "Вода (мл)");
    for (int i = 0; i < 5; ++i) {
        pFood.drawText(margin + i * stepX - 10, hFood - 10, QString("День %1").arg(i+1));
        pWater.drawText(margin + i * stepX - 10, hWater - 10, QString("День %1").arg(i+1));
    }

    pFood.setPen(QPen(QColor(70, 130, 180), 2));
    for (int i = 0; i < 4; ++i) {
        double x1 = margin + i * stepX;
        double y1 = hFood - margin - foods[i] * scaleFood;
        double x2 = margin + (i+1) * stepX;
        double y2 = hFood - margin - foods[i+1] * scaleFood;
        pFood.drawLine(x1, y1, x2, y2);
    }
    pFood.setBrush(Qt::blue);
    for (int i = 0; i < 5; ++i) {
        double x = margin + i * stepX;
        double y = hFood - margin - foods[i] * scaleFood;
        pFood.drawEllipse(QPointF(x, y), 3, 3);
    }

    pWater.setPen(QPen(QColor(34, 139, 34), 2));
    for (int i = 0; i < 4; ++i) {
        double x1 = margin + i * stepX;
        double y1 = hWater - margin - waters[i] * scaleWater;
        double x2 = margin + (i+1) * stepX;
        double y2 = hWater - margin - waters[i+1] * scaleWater;
        pWater.drawLine(x1, y1, x2, y2);
    }
    pWater.setBrush(Qt::darkGreen);
    for (int i = 0; i < 5; ++i) {
        double x = margin + i * stepX;
        double y = hWater - margin - waters[i] * scaleWater;
        pWater.drawEllipse(QPointF(x, y), 3, 3);
    }

    pFood.end();
    pWater.end();

    ui->foodGraphLabel->setPixmap(foodPix);
    ui->waterGraphLabel->setPixmap(waterPix);
}

// ==================== ВСПОМОГАТЕЛЬНЫЕ ====================
QStringList MainWindow::breedsForSpecies(const QString &speciesEng) const
{
    auto it = speciesBreeds.find(speciesEng);
    if (it != speciesBreeds.end())
        return it->keys();
    return QStringList();
}

BreedNorm MainWindow::getBreedNorm(const QString &speciesEng, const QString &breedEng) const
{
    auto itSpec = speciesBreeds.find(speciesEng);
    if (itSpec != speciesBreeds.end()) {
        auto itBreed = itSpec->find(breedEng);
        if (itBreed != itSpec->end())
            return *itBreed;
    }
    return {"Неизвестная порода", 200, 400, 0, 0, 0, 0};
}

QString MainWindow::analyzeFiveDays(const QString &speciesEng, const QString &breedEng,
                                    double weight, double age,
                                    const QStringList &diseases,
                                    const QVector<double> &foods,
                                    const QVector<double> &waters) const
{
    BreedNorm norm = getBreedNorm(speciesEng, breedEng);
    double foodNorm = norm.avgFood;
    double waterNorm = norm.avgWater;

    for (const QString &d : diseases) {
        if (seriousDiseases.contains(d))
            return "⚠ СЕРЬЁЗНОЕ ЗАБОЛЕВАНИЕ!\nНЕМЕДЛЕННО обратитесь к ветеринару!";
    }

    QStringList criticalAdvice;
    QStringList normalAdvice;

    for (int i = 0; i < 5; ++i) {
        double f = foods[i];
        double w = waters[i];
        double foodRatio = f / foodNorm;
        double waterRatio = w / waterNorm;

        if (foodRatio < 0.5) {
            criticalAdvice << QString("День %1: корм <50%% нормы (%2 г вместо %3 г) → к ветеринару!")
                              .arg(i+1).arg(f).arg(foodNorm);
        } else if (foodRatio < 0.8) {
            normalAdvice << QString("День %1: корм на %2%% ниже нормы. Увеличьте порцию.")
                              .arg(i+1).arg(qRound((1.0 - foodRatio)*100));
        } else if (foodRatio > 1.5) {
            criticalAdvice << QString("День %1: корм >150%% нормы (%2 г вместо %3 г) → к ветеринару!")
                              .arg(i+1).arg(f).arg(foodNorm);
        } else if (foodRatio > 1.2) {
            normalAdvice << QString("День %1: корм на %2%% выше нормы. Уменьшите порцию.")
                              .arg(i+1).arg(qRound((foodRatio - 1.0)*100));
        }

        if (waterRatio < 0.5) {
            criticalAdvice << QString("День %1: вода <50%% нормы (%2 мл вместо %3 мл) → к ветеринару!")
                              .arg(i+1).arg(w).arg(waterNorm);
        } else if (waterRatio < 0.8) {
            normalAdvice << QString("День %1: воды на %2%% ниже нормы. Увеличьте потребление воды.")
                              .arg(i+1).arg(qRound((1.0 - waterRatio)*100));
        } else if (waterRatio > 1.5) {
            criticalAdvice << QString("День %1: вода >150%% нормы (%2 мл вместо %3 мл) → к ветеринару!")
                              .arg(i+1).arg(w).arg(waterNorm);
        } else if (waterRatio > 1.2) {
            normalAdvice << QString("День %1: воды на %2%% выше нормы. Проверьте здоровье питомца.")
                              .arg(i+1).arg(qRound((waterRatio - 1.0)*100));
        }
    }

    if (!criticalAdvice.isEmpty()) {
        return "🚨 ТРЕВОГА! Серьёзные отклонения в питании:\n- " +
               criticalAdvice.join("\n- ") +
               "\n\nОбязательно покажите питомца ветеринару!";
    }

    if (!normalAdvice.isEmpty()) {
        for (const QString &d : diseases) {
            if (d == "obesity")
                normalAdvice << "Ожирение: снизьте калорийность, увеличьте активность.";
            else if (d == "underweight")
                normalAdvice << "Недостаток веса: усиленное питание, консультация врача.";
            else if (d == "arthritis")
                normalAdvice << "Артрит: мягкая подстилка, избегайте нагрузок.";
            else if (d == "allergy")
                normalAdvice << "Аллергия: исключите аллергены, перейдите на диету.";
        }
        return "📋 Рекомендации:\n- " + normalAdvice.join("\n- ");
    }

    return "✅ Отлично! Все показатели в норме (отклонение менее 20%). Так держать!";
}

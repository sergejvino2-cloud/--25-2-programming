// AnalysisDialog.cpp
#include "analysisdialog.h"
#include "ui_AnalysisDialog.h"
#include <QPainter>
#include <QPixmap>
#include <algorithm>

AnalysisDialog::AnalysisDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AnalysisDialog)
{
    ui->setupUi(this);
    setWindowTitle("Результаты анализа");
    setMinimumSize(750, 650);
}

AnalysisDialog::~AnalysisDialog()
{
    delete ui;
}

void AnalysisDialog::setResult(const QString& resultText)
{
    ui->resultTextEdit->setHtml(resultText);
}

void AnalysisDialog::setGraphData(const QVector<double>& foods,
                                   const QVector<double>& waters,
                                   double foodNorm,
                                   double waterNorm)
{
    drawGraphs(foods, waters, foodNorm, waterNorm);
}

void AnalysisDialog::setTitle(const QString& petInfo)
{
    ui->titleLabel->setText(petInfo);
}

void AnalysisDialog::drawGraphs(const QVector<double>& foods,
                                 const QVector<double>& waters,
                                 double foodNorm,
                                 double waterNorm)
{
    if (ui->foodGraphLabel->width() < 10 || ui->foodGraphLabel->height() < 10) {
        ui->foodGraphLabel->setMinimumSize(300, 200);
    }
    if (ui->waterGraphLabel->width() < 10 || ui->waterGraphLabel->height() < 10) {
        ui->waterGraphLabel->setMinimumSize(300, 200);
    }

    ui->foodGraphLabel->updateGeometry();
    ui->waterGraphLabel->updateGeometry();

    QApplication::processEvents();

    int wFood = ui->foodGraphLabel->width();
    int hFood = ui->foodGraphLabel->height();
    int wWater = ui->waterGraphLabel->width();
    int hWater = ui->waterGraphLabel->height();

    if (wFood < 10) wFood = 300;
    if (hFood < 10) hFood = 200;
    if (wWater < 10) wWater = 300;
    if (hWater < 10) hWater = 200;

    QPixmap foodPix(wFood, hFood);
    QPixmap waterPix(wWater, hWater);
    foodPix.fill(Qt::white);
    waterPix.fill(Qt::white);

    QPainter pFood(&foodPix);
    QPainter pWater(&waterPix);
    pFood.setRenderHint(QPainter::Antialiasing);
    pWater.setRenderHint(QPainter::Antialiasing);

    int margin = 40;

    double maxFood = *std::max_element(foods.begin(), foods.end());
    double maxWater = *std::max_element(waters.begin(), waters.end());
    maxFood = qMax(maxFood, foodNorm * 1.5);
    maxWater = qMax(maxWater, waterNorm * 1.5);

    double scaleFood = (hFood - 2.0 * margin) / maxFood;
    double scaleWater = (hWater - 2.0 * margin) / maxWater;
    double stepXFood = (wFood - 2.0 * margin) / 4.0;
    double stepXWater = (wWater - 2.0 * margin) / 4.0;

    // График корма
    pFood.setPen(QPen(QColor(255, 100, 100), 1, Qt::DashLine));
    double yNormFood = hFood - margin - foodNorm * scaleFood;
    pFood.drawLine(margin, yNormFood, wFood - margin, yNormFood);
    pFood.setPen(Qt::black);
    pFood.drawText(margin + 5, yNormFood - 5, QString("Норма: %1 г").arg(foodNorm));

    pFood.drawLine(margin, hFood - margin, wFood - margin, hFood - margin);
    pFood.drawLine(margin, margin, margin, hFood - margin);

    for (int i = 0; i < 5; ++i) {
        pFood.drawText(margin + i * stepXFood - 15, hFood - margin + 20,
                       QString("День %1").arg(i+1));
    }

    QPen foodPen(QColor(70, 130, 180), 3);
    pFood.setPen(foodPen);
    for (int i = 0; i < 4; ++i) {
        double x1 = margin + i * stepXFood;
        double y1 = hFood - margin - foods[i] * scaleFood;
        double x2 = margin + (i+1) * stepXFood;
        double y2 = hFood - margin - foods[i+1] * scaleFood;
        pFood.drawLine(x1, y1, x2, y2);
    }

    pFood.setBrush(QColor(70, 130, 180));
    for (int i = 0; i < 5; ++i) {
        double x = margin + i * stepXFood;
        double y = hFood - margin - foods[i] * scaleFood;
        pFood.drawEllipse(QPointF(x, y), 5, 5);
        pFood.drawText(x - 15, y - 10, QString::number(foods[i], 'f', 0));
    }

    // График воды
    pWater.setPen(QPen(QColor(255, 100, 100), 1, Qt::DashLine));
    double yNormWater = hWater - margin - waterNorm * scaleWater;
    pWater.drawLine(margin, yNormWater, wWater - margin, yNormWater);
    pWater.setPen(Qt::black);
    pWater.drawText(margin + 5, yNormWater - 5, QString("Норма: %1 мл").arg(waterNorm));

    pWater.drawLine(margin, hWater - margin, wWater - margin, hWater - margin);
    pWater.drawLine(margin, margin, margin, hWater - margin);

    for (int i = 0; i < 5; ++i) {
        pWater.drawText(margin + i * stepXWater - 15, hWater - margin + 20,
                        QString("День %1").arg(i+1));
    }

    QPen waterPen(QColor(34, 139, 34), 3);
    pWater.setPen(waterPen);
    for (int i = 0; i < 4; ++i) {
        double x1 = margin + i * stepXWater;
        double y1 = hWater - margin - waters[i] * scaleWater;
        double x2 = margin + (i+1) * stepXWater;
        double y2 = hWater - margin - waters[i+1] * scaleWater;
        pWater.drawLine(x1, y1, x2, y2);
    }

    pWater.setBrush(QColor(34, 139, 34));
    for (int i = 0; i < 5; ++i) {
        double x = margin + i * stepXWater;
        double y = hWater - margin - waters[i] * scaleWater;
        pWater.drawEllipse(QPointF(x, y), 5, 5);
        pWater.drawText(x - 15, y - 10, QString::number(waters[i], 'f', 0));
    }

    pFood.end();
    pWater.end();

    ui->foodGraphLabel->setPixmap(foodPix);
    ui->waterGraphLabel->setPixmap(waterPix);
}

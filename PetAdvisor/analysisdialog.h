// AnalysisDialog.h
#pragma once

#include <QDialog>
#include <QVector>
#include <QString>

namespace Ui {
class AnalysisDialog;
}

class AnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AnalysisDialog(QWidget *parent = nullptr);
    ~AnalysisDialog();

    void setResult(const QString& resultText);
    void setGraphData(const QVector<double>& foods,
                      const QVector<double>& waters,
                      double foodNorm,
                      double waterNorm);
    void setTitle(const QString& petInfo);

private:
    Ui::AnalysisDialog *ui;

    void drawGraphs(const QVector<double>& foods,
                    const QVector<double>& waters,
                    double foodNorm,
                    double waterNorm);
};

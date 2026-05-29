// main.cpp
#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QFont>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setWindowIcon(QIcon(":/resources/icons/happy.ico"));

    QFile styleFile(":/resources/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        a.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }

    QFont defaultFont = a.font();
    defaultFont.setPointSize(11);
    a.setFont(defaultFont);

    MainWindow w;
    w.show();

    return a.exec();
}

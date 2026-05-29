QT += core gui widgets concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VirtualPetApp
TEMPLATE = app

SOURCES += main.cpp mainwindow.cpp \
    analysisdialog.cpp \
    dailytracker.cpp \
    sensorhardware.cpp
HEADERS += mainwindow.h \
    analysisdialog.h \
    dailytracker.h \
    sensorhardware.h
FORMS   += mainwindow.ui \
    AnalysisDialog.ui \
    analysisdialog.ui
RESOURCES += resources.qrc
# VirtualPetApp.pro


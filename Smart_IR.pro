#-------------------------------------------------
#
# Project created by QtCreator 2017-07-10T17:39:42
#
#-------------------------------------------------

QT       += core gui
QT       += serialport
QT       += charts sql
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


TARGET = Smart_IR
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    aboutirc.cpp \
    comport_setting.cpp \
    crc32.cpp \
    protocol.cpp \
    learningwave.cpp \
    upgradedialog.cpp \
    ir_learning.cpp \
    upgradethread.cpp \
    sirlistwidget.cpp

HEADERS += \
        mainwindow.h \
    aboutirc.h \
    comport_setting.h \
    crc32.h \
    protocol.h \
    learningwave.h \
    upgradedialog.h \
    ir_learning.h \
    upgradethread.h \
    sirlistwidget.h

FORMS += \
        mainwindow.ui \
    aboutirc.ui \
    comport_setting.ui \
    learningwave.ui \
    upgradedialog.ui

RESOURCES += \
    resource.qrc

RC_ICONS += smartIRLogo.ico

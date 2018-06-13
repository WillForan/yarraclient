#-------------------------------------------------
#
# Project created by QtCreator 2018-06-11T16:18:55
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += ../Client/qtsingleapplication/src
include(../Client/qtsingleapplication/src/qtsingleapplication.pri)

TARGET = YCA
TEMPLATE = app

SOURCES += main.cpp\
        yca_mainwindow.cpp

HEADERS  += yca_mainwindow.h \
    main.h \
    yca_global.h

FORMS    += yca_mainwindow.ui

RESOURCES += yca.qrc

RC_FILE = yca.rc

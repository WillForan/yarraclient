#-------------------------------------------------
#
# Project created by QtCreator 2014-07-14T17:30:51
#
#-------------------------------------------------

QT       += core widgets gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Diagnostics
TEMPLATE = app

DEFINES+=YARRA_APP_YD

SOURCES += main.cpp \
    yd_mainwindow.cpp

HEADERS  += \ 
    yd_mainwindow.h \
    yd_global.h

FORMS    += \ 
    yd_mainwindow.ui

RESOURCES += diagnostics.qrc

RC_FILE = diagnostics.rc

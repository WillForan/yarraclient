#-------------------------------------------------
#
# Project created by QtCreator 2014-07-14T17:30:51
#
#-------------------------------------------------

QT       += core widgets gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Diagnostics
TEMPLATE = app

DEFINES+=YARRA_APP_DIAGNOSTICS

SOURCES += main.cpp \
    ../Client/rds_runtimeinformation.cpp \
    ../Client/rds_configuration.cpp \
    ../Client/rds_raid.cpp \
    ../Client/rds_log.cpp \
    ../Client/rds_exechelper.cpp \
    ../Client/rds_network.cpp \
    ../Client/rds_anonymizeVB17.cpp \
    ../NetLogger/netlogger.cpp \
    yd_mainwindow.cpp \
    yd_test.cpp \
    yd_test_systeminfo.cpp \
    yd_test_syngo.cpp \
    yd_test_yarra.cpp

HEADERS  += \ 
    ../Client/rds_runtimeinformation.h \
    ../Client/rds_configuration.h \
    ../Client/rds_raid.h \
    ../Client/rds_log.h \
    ../Client/rds_exechelper.h \
    ../Client/rds_network.h \
    ../Client/rds_anonymizeVB17.h \
    ../NetLogger/netlogger.h \
    ../NetLogger/netlog_events.h \
    yd_mainwindow.h \
    yd_global.h \
    yd_test.h \
    yd_test_systeminfo.h \
    yd_test_syngo.h \
    yd_test_yarra.h

FORMS    += \ 
    yd_mainwindow.ui

RESOURCES += diagnostics.qrc

RC_FILE = diagnostics.rc

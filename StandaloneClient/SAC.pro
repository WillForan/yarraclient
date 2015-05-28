#-------------------------------------------------
#
# Project created by QtCreator 2014-07-14T17:30:51
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SAC
TEMPLATE = app

DEFINES+=YARRA_APP_SAC

SOURCES += main.cpp\
        sac_mainwindow.cpp \
    ../Client/rds_runtimeinformation.cpp \
    ../Client/rds_log.cpp \
    ../Client/rds_exechelper.cpp \
    ../Client/rds_network.cpp \
    ../OfflineReconClient/ort_modelist.cpp \
    sac_bootdialog.cpp \
    sac_network.cpp \
    sac_copydialog.cpp \
    sac_configurationdialog.cpp

HEADERS  += sac_mainwindow.h \
    ../Client/rds_runtimeinformation.h \
    ../Client/rds_log.h \
    ../Client/rds_exechelper.h \
    ../Client/rds_network.h \
    ../OfflineReconClient/ort_modelist.h \
    ../OfflineReconClient/ort_returnonfocus.h \
    sac_global.h \
    sac_bootdialog.h \
    sac_network.h \
    sac_copydialog.h \
    sac_configurationdialog.h

FORMS    += sac_mainwindow.ui \
    sac_bootdialog.ui \
    sac_copydialog.ui \
    sac_configurationdialog.ui

RESOURCES += \
    sac.qrc

RC_FILE = sac.rc

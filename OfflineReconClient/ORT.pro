#-------------------------------------------------
#
# Project created by QtCreator 2013-11-05T19:01:01
#
#-------------------------------------------------

QT       +=  core widgets gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ORT
TEMPLATE = app

INCLUDEPATH += ../Client/qtsingleapplication/src
include(../Client/qtsingleapplication/src/qtsingleapplication.pri)

DEFINES+=YARRA_APP_ORT

SOURCES += main.cpp \
    ort_mainwindow.cpp \
    ../Client/rds_runtimeinformation.cpp \
    ../Client/rds_anonymizeVB17.cpp \
    ../Client/rds_configuration.cpp \
    ../Client/rds_raid.cpp \
    ../Client/rds_log.cpp \
    ort_confirmationdialog.cpp \
    ort_modelist.cpp \
    ort_waitdialog.cpp \
    ort_copydialog.cpp \
    ort_network.cpp \
    ../Client/rds_exechelper.cpp \
    ../Client/rds_network.cpp \
    ort_recontask.cpp \
    ort_bootdialog.cpp

HEADERS  += \
    ort_mainwindow.h \
    ort_global.h \
    ../Client/rds_runtimeinformation.h \
    ../Client/rds_anonymizeVB17.h \
    ../Client/rds_configuration.h \
    ../Client/rds_raid.h \
    ../Client/rds_log.h \
    ort_confirmationdialog.h \
    ort_modelist.h \
    ort_waitdialog.h \
    ort_copydialog.h \
    ort_network.h \
    ../Client/rds_exechelper.h \
    ../Client/rds_network.h \
    ort_recontask.h \
    ort_bootdialog.h

FORMS    += \
    ort_mainwindow.ui \
    ort_confirmationdialog.ui \
    ort_waitdialog.ui \
    ort_copydialog.ui \
    ort_bootdialog.ui

RESOURCES += \
    ort.qrc

RC_FILE = ort.rc

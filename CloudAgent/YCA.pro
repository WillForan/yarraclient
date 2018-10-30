#-------------------------------------------------
#
# Project created by QtCreator 2018-06-11T16:18:55
#
#-------------------------------------------------

QT += core widgets gui xml svg
#testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += ../Client/qtsingleapplication/src
include(../Client/qtsingleapplication/src/qtsingleapplication.pri)

TARGET = YCA
TEMPLATE = app
DEFINES+=YARRA_APP_YCA

SOURCES += main.cpp\
        yca_mainwindow.cpp \
    ../Client/rds_runtimeinformation.cpp \
    ../CloudTools/yct_configuration.cpp \
    ../CloudTools/yct_aws/qtawsqnam.cpp \
    ../CloudTools/yct_aws/qtaws.cpp \
    yca_transferindicator.cpp \
    ../CloudTools/yct_api.cpp \
    yca_task.cpp \
    yca_threadlog.cpp

HEADERS  += yca_mainwindow.h \
    main.h \
    ../Client/rds_runtimeinformation.h \
    yca_global.h \
    ../CloudTools/yct_common.h \
    ../CloudTools/yct_configuration.h \
    ../CloudTools/yct_aws/qtawsqnam.h \
    ../CloudTools/yct_aws/qtaws.h \
    ../CloudTools/yct_api.h \
    yca_transferindicator.h \
    yca_task.h \
    yca_threadlog.h

FORMS    += yca_mainwindow.ui \
    yca_transferindicator.ui

RESOURCES += yca.qrc

RC_FILE = yca.rc

#-------------------------------------------------
#
# Project created by QtCreator 2018-06-11T16:18:55
#
#-------------------------------------------------

QT += core widgets gui xml svg testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += ../Client/qtsingleapplication/src
include(../Client/qtsingleapplication/src/qtsingleapplication.pri)

TARGET = YCA
TEMPLATE = app

SOURCES += main.cpp\
        yca_mainwindow.cpp \
    ../CloudTools/yct_configuration.cpp \
    ../CloudTools/yct_aws/qtawsqnam.cpp \
    ../CloudTools/yct_aws/qtaws.cpp \
    yca_transferindicator.cpp

HEADERS  += yca_mainwindow.h \
    main.h \
    yca_global.h \
    ../CloudTools/yct_common.h \
    ../CloudTools/yct_configuration.h \
    ../CloudTools/yct_aws/qtawsqnam.h \
    ../CloudTools/yct_aws/qtaws.h \
    yca_transferindicator.h

FORMS    += yca_mainwindow.ui \
    yca_transferindicator.ui

RESOURCES += yca.qrc

RC_FILE = yca.rc

#-------------------------------------------------
#
# Project created by QtCreator 2011-12-12T17:01:16
#
#-------------------------------------------------

QT += core widgets gui xml svg

DEFINES += YARRA_APP_RDS
#DEFINES += NETLOGGER_DISABLE_DOMAIN_VALIDATION

TARGET    = RDS
TEMPLATE  = app
RESOURCES = rds.qrc
RC_FILE   = rds.rc

INCLUDEPATH += qtsingleapplication/src
include(qtsingleapplication/src/qtsingleapplication.pri)

SOURCES += main.cpp \
        rds_configurationwindow.cpp \
        rds_runtimeinformation.cpp \
        rds_global.cpp \
        rds_operationwindow.cpp \
        rds_configuration.cpp \
        rds_network.cpp \
        rds_log.cpp \
        rds_raid.cpp \
        rds_processcontrol.cpp \
        rds_activitywindow.cpp \
        rds_debugwindow.cpp \
        rds_anonymizeVB17.cpp \
        rds_copydialog.cpp \
        rds_checksum.cpp \
        ../NetLogger/netlogger.cpp \
    rds_iconwindow.cpp \
    rds_exechelper.cpp \
    rds_mailbox.cpp \
    rds_mailboxwindow.cpp \
    rds_mailboxmessage.cpp

HEADERS  += rds_configurationwindow.h \
            rds_runtimeinformation.h \
            rds_global.h \
            rds_operationwindow.h \
            rds_configuration.h \
            rds_network.h \
            rds_log.h \
            rds_raid.h \
            rds_processcontrol.h \
            rds_activitywindow.h \
            rds_debugwindow.h \
            rds_anonymizeVB17.h \
            rds_copydialog.h \
            main.h \
            rds_checksum.h \
            ../NetLogger/netlogger.h \
            ../NetLogger/netlog_events.h \
    rds_iconwindow.h \
    rds_exechelper.h \
    rds_mailbox.h \
    rds_mailboxwindow.h \
    rds_mailboxmessage.h

FORMS    += rds_configurationwindow.ui \
            rds_operationwindow.ui \
            rds_activitywindow.ui \
            rds_debugwindow.ui \
            rds_copydialog.ui \
    rds_iconwindow.ui \
    rds_mailboxwindow.ui

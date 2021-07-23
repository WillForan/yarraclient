QT += core
QT -= gui

CONFIG += c++11

TARGET = twixchecker
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
           ../yct_prepare/yct_twix_anonymizer.cpp

HEADERS += \
    ../yct_prepare/yct_twix_anonymizer.h \
    ../yct_prepare/yct_twix_header.h

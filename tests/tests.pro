QT += core testlib
QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = tst_studystats

INCLUDEPATH += ..

SOURCES += \
    studystats_test.cpp \
    ../utils/studystats.cpp

HEADERS += \
    ../utils/studystats.h \
    ../datastruct.h

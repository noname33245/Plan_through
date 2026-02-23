QT       += core gui
QT       += network
QT       += charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += appdatas.cpp \
    main.cpp \
    mainwindow.cpp \
    memreduct.cpp \
    utils/datehelper.cpp \
    utils/widgetcontainer.cpp \
    widgets/dayview.cpp \
    widgets/monthview.cpp \
    widgets/timeaxis.cpp

HEADERS += appdatas.h \
    datastruct.h \
    mainwindow.h \
    memreduct.h \
    utils/datehelper.h \
    utils/widgetcontainer.h \
    widgets/dayview.h \
    widgets/monthview.h \
    widgets/timeaxis.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    Plan_through_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
RC_ICONS = app.ico

# 使用普通用户权限
CONFIG(debug, debug|release) {
    win32-g++: RC_FILE += app-debug.rc
} else {
    win32-g++: RC_FILE += app.rc
}
win32-msvc: QMAKE_LFLAGS += /MANIFESTUAC:"level='asInvoker' uiAccess='false'"
win32-g++: QMAKE_LFLAGS += -mwindows

RESOURCES += res.qrc

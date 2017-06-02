QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = yaha
TEMPLATE = app

include(../../yaha.pri)

SOURCES += main.cpp\
        mainwindow.cpp \
        device.cpp

HEADERS  += mainwindow.h \
        device.h
INCLUDEPATH += ../../include

FORMS    += mainwindow.ui

QMAKE_TARGET_COMPANY = undersampled bananas
QMAKE_TARGET_PRODUCT = Yaha example
QMAKE_TARGET_DESCRIPTION = "Yaha example"

RESOURCES += \
    untitled.qrc

RC_ICONS += usb.ico

DISTFILES +=

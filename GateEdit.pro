#-------------------------------------------------
#
# Project created by QtCreator 2016-01-20T15:10:39
#
#-------------------------------------------------

QT       += core gui xml network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GateEdit
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    xmllibrary.cpp \
    cidentry.cpp \
    csegtab.cpp

HEADERS  += mainwindow.h \
    xmllibrary.h \
    cidentry.h \
    csegtab.h

FORMS    += mainwindow.ui

TRANSLATIONS += gateedit_ru.ts

RESOURCES += \
    resource.qrc

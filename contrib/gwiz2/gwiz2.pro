#-------------------------------------------------
#
# Project created by QtCreator 2014-03-22T12:46:39
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += precompile_header

debug {
    DEFINES += DEBUG
    MOC_DIR = ./generatedfiles/debug
    OBJECTS_DIR = debug
}
release {
    DEFINES += NDEBUG
    DEFINES += QT_NO_DEBUG_OUTPUT
    DEFINES += QT_NO_WARNING_OUTPUT
    MOC_DIR = ./generatedfiles/release
    OBJECTS_DIR = release
}
UI_DIR = ./generatedfiles
RCC_DIR = ./generatedfiles
TARGET = gwiz2
TEMPLATE = app

PRECOMPILED_HEADER = stdafx.h

INCLUDEPATH += ../../include ./generatedfiles

LIBS += -L ../../lib ../../lib/libinterpbaseclass.so -lGL -lGLU

SOURCES += main.cpp\
        mainwindow.cpp \
    gwizard.cpp \
    wizard.cpp \
    wizfile.cpp \
    wizline.cpp \
    wizlinetext.cpp \
    wizlineoword.cpp \
    mru.cpp \
    gcodepreviewglwidget.cpp \
    utility.cpp \
    glnavbase.cpp \
    glutility.cpp \
    hersheyfont.cpp \
    applicationsettings.cpp \
    parameter.cpp \
    parameternumeric.cpp \
    parameterlist.cpp \
    gwizlineedit.cpp \
    gwizlistview.cpp \
    glinterp.cpp

HEADERS  += mainwindow.h \
    gwizard.h \
    stdafx.h \
    wizard.h \
    wizfile.h \
    wizline.h \
    wizlinetext.h \
    wizlineoword.h \
    mru.h \
    gcodepreviewglwidget.h \
    utility.h \
    common.h \
    glnavbase.h \
    glutility.h \
    hersheyfont.h \
    applicationsettings.h \
    parameter.h \
    parameternumeric.h \
    parameterlist.h \
    gwizlineedit.h \
    gwizlistview.h \
    glinterp.h

FORMS    += mainwindow.ui

RESOURCES += \
    gwiz2.qrc

OTHER_FILES += \
    gwiz2.ini

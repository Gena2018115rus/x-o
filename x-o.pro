QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += strict_c++
CONFIG += console

#QMAKE_CXXFLAGS += -std=c++17
#QMAKE_CXXFLAGS += -pedantic
#QMAKE_CXXFLAGS += -Os
#QMAKE_CXXFLAGS += -Wall
QMAKE_CXXFLAGS += -Wno-deprecated-copy

win32:LIBS += libopengl32


# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
#DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    clientwindow.cpp \
    eth-lib/eth-lib.cpp \
    main.cpp \
    mainwindow.cpp \
    waitdialog.cpp

HEADERS += \
    clientwindow.h \
    eth-lib/eth-lib.hpp \
    mainwindow.h \
    openglwidget.h \
    waitdialog.h

FORMS += \
    clientwindow.ui \
    mainwindow.ui \
    waitdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

RESOURCES += \
    shaders.qrc

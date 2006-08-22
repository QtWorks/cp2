TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG	+= debug

INCLUDEPATH	+= ../

HEADERS	+= QtDSP.h

SOURCES	+= main.cpp
SOURCES	+= ../CP2Classes/fifo.cpp

FORMS	= QtDSPBase.ui

INCLUDEPATH += ../../
INCLUDEPATH += ../../Qttoolbox
INCLUDEPATH += ../CP2Classes

SOURCES += QtDSP.cpp


win32 {
   LIBS       += opengl32.lib
   LIBS       += glu32.lib
}

CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions




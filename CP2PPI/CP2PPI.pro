TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG	+= debug

INCLUDEPATH	+= ../

HEADERS	+= CP2PPI.h

SOURCES	+= main.cpp

FORMS	= CP2PPIBase.ui

INCLUDEPATH += ../../
INCLUDEPATH += ../../Qttoolbox


SOURCES += CP2PPI.cc


win32 {
      LIBS       += ../../Qttoolbox/PPI/Debug/PPI.lib
   LIBS       += ../../Qttoolbox/ColorBar/Debug/ColorBar.lib
   LIBS       += opengl32.lib
   LIBS       += glu32.lib
}

CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions


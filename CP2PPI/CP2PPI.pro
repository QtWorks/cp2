TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += debug
CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions

HEADERS += CP2PPI.h
HEADERS += PpiInfo.h

SOURCES += CP2PPI.cpp
SOURCES += PpiInfo.cpp
SOURCES += main.cpp

FORMS	= CP2PPIBase.ui

INCLUDEPATH	+= ../
INCLUDEPATH += ../../
INCLUDEPATH += ../../Qttoolbox
INCLUDEPATH += ../CP2Net

LIBS += ../../Qttoolbox/PPI/Debug/PPI.lib
LIBS += ../../Qttoolbox/ColorBar/Debug/ColorBar.lib
LIBS += ../CP2Net/Release/CP2Net.lib
LIBS += opengl32.lib
LIBS += glu32.lib

DESTDIR = Release




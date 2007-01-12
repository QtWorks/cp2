TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += debug
CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions

HEADERS += CP2PPI.h
HEADERS += PpiInfo.h
HEADERS += MomentThread.h

SOURCES += CP2PPI.cpp
SOURCES += PpiInfo.cpp
SOURCES += MomentThread.cpp
SOURCES += main.cpp

FORMS	= CP2PPIBase.ui

INCLUDEPATH	+= ../
INCLUDEPATH += ../../
INCLUDEPATH += ../../fftw3.1
INCLUDEPATH += ../../Qttoolbox
INCLUDEPATH += ../CP2Net
INCLUDEPATH += ../Moments

LIBS += ../../Qttoolbox/PPI/Debug/PPI.lib
LIBS += ../../Qttoolbox/ColorBar/Debug/ColorBar.lib
LIBS += ../Moments/Debug/Moments.lib
LIBS += ../CP2Net/Debug/CP2Net.lib
LIBS += ../../fftw3.1/libfftw3-3.lib
LIBS += opengl32.lib
LIBS += glu32.lib

DESTDIR = Debug




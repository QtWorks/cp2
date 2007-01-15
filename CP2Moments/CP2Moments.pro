TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += debug
CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions

HEADERS += CP2Moments.h
HEADERS += MomentThread.h
HEADERS += ../include/dd_types.h

SOURCES += main.cpp
SOURCES += CP2Moments.cpp
SOURCES += MomentThread.cpp

FORMS	= CP2MomentsBase.ui

INCLUDEPATH	+= ../
INCLUDEPATH += ../../
INCLUDEPATH += ../../fftw3.1
INCLUDEPATH += ../../Qttoolbox
INCLUDEPATH += ../CP2Net
INCLUDEPATH += ../Moments

LIBS += ../Moments/Release/Moments.lib
LIBS += ../CP2Net/Release/CP2Net.lib
LIBS += ../../fftw3.1/libfftw3-3.lib

DESTDIR = Release





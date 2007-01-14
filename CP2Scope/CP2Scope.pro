TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions
CONFIG += debug

FORMS	= .\CP2ScopeBase.ui

SOURCES += CP2Scope.cpp
SOURCES += PlotInfo.cpp
SOURCES += main.cpp

HEADERS += CP2Scope.h
Headers += PlotInfo.h

INCLUDEPATH += ../CP2Net
INCLUDEPATH += ../Moments
INCLUDEPATH += ../../
INCLUDEPATH += ../../QtToolbox
INCLUDEPATH += ../../fftw3.1
INCLUDEPATH += ../../qwt/include

DESTDIR = Debug

IMAGES	= images/editcopy \
	images/editcut \
	images/editpaste \
	images/filenew \
	images/fileopen \
	images/filesave \
	images/print \
	images/redo \
	images/searchfind \
	images/undo

LIBS += ../../Qttoolbox/ScopePlot/Debug/ScopePlot.lib
LIBS += ../../Qttoolbox/TwoKnobs/Debug/TwoKnobs.lib
LIBS += ../../fftw3.1/libfftw3-3.lib
LIBS += ../Moments/Release/Moments.lib
LIBS += ../CP2Net/Release/CP2Net.lib


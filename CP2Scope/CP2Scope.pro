TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions
CONFIG += debug

FORMS	= .\CP2ScopeBase.ui

SOURCES += CP2Scope.cpp
SOURCES += ../CP2exec/CP2Net.cpp
SOURCES += main.cpp

HEADERS += CP2Scope.h
HEADERS += ../CP2exec/CP2Net.h

INCLUDEPATH += ../CP2exec
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
LIBS += ../../Qttoolbox/Knob/Debug/Knob.lib
LIBS += ../../fftw3.1/libfftw3-3.lib
LIBS += ../Moments/Debug/Moments.lib

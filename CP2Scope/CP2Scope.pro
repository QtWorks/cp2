TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions
CONFIG += debug

FORMS	= .\CP2ScopeBase.ui

SOURCES += CP2Scope.cpp
SOURCES += main.cpp

HEADERS += CP2Scope.h

INCLUDEPATH += ../../
INCLUDEPATH += ../../QtToolbox
INCLUDEPATH += ../../fftw3.1
INCLUDEPATH += ../../qwt/include

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

win32 {
   LIBS       += ../../Qttoolbox/ScopePlot/ScopePlot.lib
   LIBS       += ../../Qttoolbox/Knob/Knob.lib
   LIBS       += ../../fftw3.1/libfftw3-3.lib
}


TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += release
CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions

HEADERS += CP2Exec.h
HEADERS += CP2ExecThread.h
HEADERS += CP2PIRAQ.h
HEADERS += MomentThread.h
HEADERS += ../include/dd_types.h
HEADERS += ../CP2Piraq/piraqComm.h

SOURCES += main.cpp
SOURCES += CP2Exec.cpp
SOURCES += CP2ExecThread.cpp
SOURCES += MomentThread.cpp
SOURCES += CP2PIRAQ.cpp
SOURCES += ../CP2Piraq/piraqComm.c

FORMS	= CP2ExecBase.ui

INCLUDEPATH	+= ../
INCLUDEPATH += ../../
INCLUDEPATH += ../../fftw3.1
INCLUDEPATH += ../../Qttoolbox
INCLUDEPATH += ../CP2Net
INCLUDEPATH += ../CP2Lib
INCLUDEPATH += ../CP2Piraq
INCLUDEPATH += ../CP2PPI
INCLUDEPATH += ../PiraqDriver
INCLUDEPATH += ../Moments
INCLUDEPATH += C:/Plx/PciSdk/Inc
INCLUDEPATH += "C:/Program Files/TVicSoft/TVicHW50/MSVC"

LIBS += ../Moments/Release/Moments.lib
LIBS += ../CP2Net/Release/CP2Net.lib
LIBS += ../../fftw3.1/libfftw3-3.lib
LIBS += C:/Plx/PciSdk/Win32/Api/Debug/plxapi.lib
LIBS += "C:/Program Files/TVicSoft/TVicHW50/MSVC/TVicHW32.lib"
LIBS += ws2_32.lib

DESTDIR = Debug




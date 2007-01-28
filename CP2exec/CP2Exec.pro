TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions
CONFIG += embed_manifest_exe

QT     += network

CONFIG(release, debug|release) {
  LIBS += ../Moments/release/Moments.lib
  LIBS += ../CP2Net/release/CP2Net.lib
  LIBS += ../PiraqDriver/release/PiraqIII_RevD_Driver.lib
} else {
  LIBS += ../Moments/debug/Moments.lib
  LIBS += ../CP2Net/debug/CP2Net.lib
  LIBS += ../PiraqDriver/debug/PiraqIII_RevD_Driver.lib
}

LIBS += "\"c:/Program Files/TVicSoft/TVicHW50/MSVC/TVicHW32.lib\""
LIBS += ../../fftw3.1/libfftw3-3.lib
LIBS += C:/Plx/PciSdk/Win32/Api/Debug/plxapi.lib

HEADERS += CP2Exec.h
HEADERS += CP2ExecThread.h
HEADERS += CP2PIRAQ.h
HEADERS += ../include/dd_types.h
HEADERS += ../CP2Piraq/piraqComm.h

SOURCES += main.cpp
SOURCES += CP2Exec.cpp
SOURCES += CP2ExecThread.cpp
SOURCES += CP2PIRAQ.cpp
SOURCES += ../CP2Piraq/piraqComm.c

FORMS	= CP2Exec.ui

INCLUDEPATH	+= ../
INCLUDEPATH += ../../
INCLUDEPATH += ../CP2Net
INCLUDEPATH += ../CP2Lib
INCLUDEPATH += ../CP2Piraq
INCLUDEPATH += ../PiraqDriver
INCLUDEPATH += ../Moments
INCLUDEPATH += C:/Plx/PciSdk/Inc
INCLUDEPATH += "C:/Program Files/TVicSoft/TVicHW50/MSVC"



TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions
CONFIG += opengl

QT     += network
QT     += opengl

CONFIG(release, debug|release) {
  LIBS += ../../Qttoolbox/PPI/release/PPI.lib
  LIBS += ../CP2Net/release/CP2Net.lib
  LIBS += c:/Projects/Qwt/lib/qwt5.lib
} else {
  LIBS += ../../Qttoolbox/PPI/debug/PPId.lib
  LIBS += ../CP2Net/debug/CP2Net.lib
  LIBS += c:/Projects/Qwt/lib/qwt5d.lib
}
LIBS += ../../fftw3.1/libfftw3-3.lib
LIBS += ws2_32.lib

HEADERS += CP2PPI.h
HEADERS += PpiInfo.h

SOURCES += CP2PPI.cpp
SOURCES += PpiInfo.cpp
SOURCES += main.cpp

FORMS	= CP2PPIBase.ui

INCLUDEPATH += ../CP2Net
INCLUDEPATH += ../../
INCLUDEPATH += c:/Projects/QtToolbox/Knob
INCLUDEPATH += c:/Projects/QtToolbox/ScopePlot
INCLUDEPATH += c:/Projects/fftw3.1
INCLUDEPATH += c:/Projects/qwt/src




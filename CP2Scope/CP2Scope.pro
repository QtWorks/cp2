TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions

QT     += network

CONFIG(release, debug|release) {
  LIBS += ../../Qttoolbox/ScopePlot/release/ScopePlot.lib
  LIBS += ../../Qttoolbox/Knob/release/Knob.lib
  LIBS += c:/Projects/Qwt/lib/qwt5.lib
  LIBS += ../CP2Net/release/CP2Net.lib
  LIBS += ../CP2Lib/release/CP2Lib.lib
  LIBS += ../CP2Config/release/CP2Config.lib
} else {
  LIBS += ../../Qttoolbox/ScopePlot/debug/ScopePlotd.lib
  LIBS += ../../Qttoolbox/Knob/debug/Knobd.lib
  LIBS += c:/Projects/Qwt/lib/qwt5d.lib
  LIBS += ../CP2Net/debug/CP2Netd.lib
  LIBS += ../CP2Lib/debug/CP2Libd.lib
  LIBS += ../CP2Config/debug/CP2Configd.lib
}
LIBS += ../../fftw3.1/libfftw3-3.lib
LIBS += ws2_32.lib

SOURCES += CP2Scope.cpp
SOURCES += PlotInfo.cpp
SOURCES += main.cpp

HEADERS += CP2Scope.h
HEADERS += PlotInfo.h
HEADERS += CP2ScopeIcon.h

FORMS	= CP2Scope.ui

RC_FILE += CP2Scope.rc

INCLUDEPATH += ../../
INCLUDEPATH += ../CP2Net
INCLUDEPATH += ../CP2Lib
INCLUDEPATH += ../CP2Config
INCLUDEPATH += ../../QtToolbox/Knob
INCLUDEPATH += ../../QtToolbox/ScopePlot
INCLUDEPATH += c:/Projects/qwt/src
INCLUDEPATH += ../../fftw3.1



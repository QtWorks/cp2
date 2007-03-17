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
  LIBS += ../../Qttoolbox/ColorBar/release/ColorBar.lib
  LIBS += ../CP2Net/release/CP2Net.lib
  LIBS += ../CP2Config/release/CP2Config.lib
  LIBS += c:/Projects/Qwt/lib/qwt5.lib
} else {
  LIBS += ../../Qttoolbox/PPI/debug/PPId.lib
  LIBS += ../../Qttoolbox/ColorBar/debug/ColorBard.lib
  LIBS += ../CP2Net/debug/CP2Netd.lib
  LIBS += ../CP2Config/debug/CP2Configd.lib
  LIBS += c:/Projects/Qwt/lib/qwt5d.lib
}
LIBS += ws2_32.lib

HEADERS += CP2PPI.h
HEADERS += ColorBarSettings.h
HEADERS += PpiInfo.h

SOURCES += CP2PPI.cpp
SOURCES += ColorBarSettings.cpp
SOURCES += PpiInfo.cpp
SOURCES += main.cpp

FORMS	+= CP2PPI.ui
FORMS	+= ColorBarSettings.ui

INCLUDEPATH += ../CP2Net
INCLUDEPATH += ../CP2Config
INCLUDEPATH += ../../
INCLUDEPATH += ../../QtToolbox/PPI
INCLUDEPATH += ../../QtToolbox/ColorBar
INCLUDEPATH += ../../QtToolbox/




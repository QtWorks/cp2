TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions

QT     += network

CONFIG(release, debug|release) {
  LIBS += ../CP2Net/Release/CP2Net.lib
  LIBS += ../CP2Lib/release/CP2Lib.lib
  LIBS += ../CP2Config/Release/CP2Config.lib
} else {
  LIBS += ../CP2Net/Debug/CP2Netd.lib
  LIBS += ../CP2Lib/debug/CP2Libd.lib
  LIBS += ../CP2Config/Debug/CP2Configd.lib
}
LIBS += ws2_32.lib

HEADERS += CP2Sim.h
HEADERS += CP2SimThread.h

SOURCES += main.cpp
SOURCES += CP2Sim.cpp
SOURCES += CP2SimThread.cpp

FORMS	= CP2Sim.ui

INCLUDEPATH	+= ../
INCLUDEPATH += ../../
INCLUDEPATH += ../CP2Net
INCLUDEPATH += ../CP2Config
INCLUDEPATH += ../CP2Lib






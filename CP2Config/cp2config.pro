TEMPLATE	= vclib
LANGUAGE	= C++

CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions
CONFIG += staticlib

QT     += 

CONFIG(release, debug|release) {
  TARGET = CP2Config
} else {
  TARGET = CP2Configd
}

HEADERS += CP2Config.h

SOURCES += CP2Config.cpp




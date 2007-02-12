TEMPLATE	= vclib
LANGUAGE	= C++

CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions
CONFIG += staticlib

QT     += network

CONFIG(debug, debug|release) {
  TARGET = CP2Netd
  DESTDIR = ./debug
} else {
  TARGET = CP2Net 
  DESTDIR = ./release
}
LIBS += ws2_32.lib

HEADERS += CP2Net.h
HEADERS += CP2UdpSocket.h

SOURCES += CP2Net.cpp
SOURCES += CP2UdpSocket.cpp

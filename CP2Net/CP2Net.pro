TEMPLATE	= vclib
LANGUAGE	= C++

CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions

QT     += network

CONFIG(release, debug|release) {
} else {
}
LIBS += ws2_32.lib

HEADERS += CP2Net.h
HEADERS += CP2UdpSocket.h

SOURCES += CP2Net.cpp
SOURCES += CP2UdpSocket.cpp

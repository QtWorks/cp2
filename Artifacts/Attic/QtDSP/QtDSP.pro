TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += debug
CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions

INCLUDEPATH	+= ../

HEADERS	+= QtDSP.h
HEADERS	+= Fifo.h

SOURCES	+= main.cpp
SOURCES	+= Fifo.cc

FORMS	= QtDSPBase.ui

INCLUDEPATH += ../../
INCLUDEPATH += ../../Qttoolbox

SOURCES += QtDSP.cpp

DESTDIR = Debug





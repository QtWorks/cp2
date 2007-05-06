TEMPLATE	= vcapp
LANGUAGE	= C++

CONFIG += qt 
CONFIG += thread
CONFIG += warn_on 
CONFIG += exceptions

QT     += network

CONFIG(release, debug|release) {
  LIBS += ../Moments/Release/Moments.lib
  LIBS += ../CP2Net/Release/CP2Net.lib
  LIBS += ../CP2Lib/release/CP2Lib.lib
  LIBS += ../CP2Config/Release/CP2Config.lib
} else {
  LIBS += ../Moments/Debug/Momentsd.lib
  LIBS += ../CP2Net/Debug/CP2Netd.lib
  LIBS += ../CP2Lib/debug/CP2Libd.lib
  LIBS += ../CP2Config/Debug/CP2Configd.lib
}
LIBS += ../../fftw3.1/libfftw3-3.lib
LIBS += ws2_32.lib

HEADERS += CP2Moments.h
HEADERS += MomentThread.h
HEADERS += CP2MomentsIcon.h
HEADERS += Stk.h
HEADERS += Filter.h
HEADERS += BiQuad.h
HEADERS += ../include/dd_types.h

SOURCES += main.cpp
SOURCES += CP2Moments.cpp
SOURCES += MomentThread.cpp
SOURCES += Stk.cpp
SOURCES += Filter.cpp
SOURCES += BiQuad.cpp

FORMS	= CP2Moments.ui

RC_FILE += CP2Moments.rc

INCLUDEPATH	+= ../
INCLUDEPATH += ../../
INCLUDEPATH += ../../fftw3.1
INCLUDEPATH += ../CP2Net
INCLUDEPATH += ../CP2Config
INCLUDEPATH += ../CP2Lib
INCLUDEPATH += ../Moments






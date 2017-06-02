# qmake include file for building Yaha as part of another project
# to use, include in your .pro file
# include(some/dir/yaha.pri)

INCLUDEPATH += $$PWD/include

SOURCES     += $$PWD/src/hidapi.cpp $$PWD/src/hiddevice.cpp
HEADERS     += $$PWD/include/hidapi.h $$PWD/include/hiddevice.h

CONFIG      += c++11
LIBS        += -lsetupapi -lhid

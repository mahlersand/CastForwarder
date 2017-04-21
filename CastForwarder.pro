TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    multidirection.cpp

unix:INCLUDEPATH += ../CastForwarder/PcapPlusPlus/Dist/header /usr/include/netinet
unix:LIBS += -L../CastForwarder/PcapPlusPlus/Dist -lPcap++ -lPacket++ -lCommon++ -lpcap -lpthread
unix:QMAKE_LFLAGS += -static-libstdc++

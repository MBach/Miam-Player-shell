QT -= gui
QT += axserver

CONFIG += qt \
    dll

LIBS += -luser32 -lole32 -loleaut32 -lgdi32 -luuid

# qaxserver.def and qaxserver.rc come from <qt>\src\activeqt\control
DEF_FILE = qaxserver.def
RC_FILE = qaxserver.rc

# ##
TARGET = MiamShell

TEMPLATE = lib

DEFINES += SHELLEXT_OVERLAY_LIBRARY

SOURCES += shelloverlay.cpp \
    dllmain.cpp

HEADERS += shelloverlay.h \
    shellext_overlay_global.h

OTHER_FILES += qaxserver.rc \
    qaxserver.def \
    qtdemo.ico

QT -= gui

TARGET   = $$qtLibraryTarget(MiamPlayerShell)
TEMPLATE = lib

MiamPlayerBuildDirectory = C:\dev\Miam-Player-build-x64\MiamPlayer

CONFIG  += c++11
CONFIG(debug, debug|release) {
    target.path = $$MiamPlayerBuildDirectory\debug\
}

CONFIG(release, debug|release) {
    target.path = $$MiamPlayerBuildDirectory\release\
}

LIBS += -lshell32 -lcomctl32 -lshlwapi -lmsimg32 -ladvapi32 -lole32
LIBS += -luser32 -loleaut32 -lgdi32 -luuid

INSTALLS += target

HEADERS += Bitmap.h \
    MiamPlayerShell.h \
    resource.h

SOURCES += Bitmap.cpp \
    MiamPlayerShell.cpp

OTHER_FILES += export.def \
    MiamPlayerShell.rc

RC_FILE = MiamPlayerShell.rc

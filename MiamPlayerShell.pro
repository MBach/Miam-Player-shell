QT      += widgets multimedia

TARGET   = $$qtLibraryTarget(MiamPlayerShell)
TEMPLATE = lib

MiamPlayerBuildDirectory = C:\dev\Miam-Player-build-x64\MiamPlayer

DEFINES += MIAM_PLUGIN

CONFIG  += c++11
CONFIG(debug, debug|release) {
    target.path = $$MiamPlayerBuildDirectory\debug\plugins
    LIBS += -Ldebug -lMiamCore
}

CONFIG(release, debug|release) {
    target.path = $$MiamPlayerBuildDirectory\release\plugins
    LIBS += -Lrelease -lMiamCore
}

INSTALLS += target

HEADERS += basicplugininterface.h \
    miamcore_global.h \
    settings.h \
    filehelper.h \
    miamplayershell.h \
    mediaplayerplugininterface.h \
    mediaplayer.h \
    listwidget.h

SOURCES += miamplayershell.cpp \
    listwidget.cpp

FORMS += config.ui

RESOURCES += \
    resources.qrc

QT += core gui widgets multimedia svg network

CONFIG += c++11

TEMPLATE = app
TARGET = OVRPhoneBridge

RESOURCES += resources.qrc

VERSION = 1.2.0

DISTFILES += .astylerc

win32 {
    QMAKE_TARGET_PRODUCT = $$TARGET
    QMAKE_TARGET_COPYRIGHT = "Copyright (c) 2019 Desuuuu"
    RC_ICONS = images/icons/icon.ico
}

macx:ICON = images/icons/icon.icns

CONFIG(debug, debug|release) {
    DEFINES += QT_SHAREDPOINTER_TRACK_POINTERS
}

include(spdlog.pri)
include(libsodium.pri)
include(openvr.pri)
include(eigen.pri)
include(sources.pri)

write_file($$top_builddir/version.txt, VERSION)

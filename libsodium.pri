INCLUDEPATH += thirdparty/libsodium/src/libsodium/include
DEPENDPATH += thirdparty/libsodium/src/libsodium/include

contains(QT_ARCH, x86_64) {
    win32: LIBS += -L"$$top_srcdir/thirdparty/libsodium/libsodium-win64/lib"
} else {
    win32: LIBS += -L"$$top_srcdir/thirdparty/libsodium/libsodium-win32/lib"
}

LIBS += -lsodium

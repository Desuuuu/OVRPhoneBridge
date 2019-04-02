INCLUDEPATH += thirdparty/libsodium/src/libsodium/include
DEPENDPATH += thirdparty/libsodium/src/libsodium/include

contains(QT_ARCH, x86_64) {
    win32: LIBS += -L"$$top_srcdir/thirdparty/libsodium/lib/win64"
} else {
    win32: LIBS += -L"$$top_srcdir/thirdparty/libsodium/lib/win32"
}

LIBS += -lsodium

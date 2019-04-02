INCLUDEPATH += thirdparty/openvr/headers
DEPENDPATH += thirdparty/openvr/headers

contains(QT_ARCH, x86_64) {
    win32: LIBS += -L"$$top_srcdir/thirdparty/openvr/lib/win64"
    else:macx: LIBS += -L"$$top_srcdir/thirdparty/openvr/lib/osx32"
    else:unix: LIBS += -L"$$top_srcdir/thirdparty/openvr/lib/linux64"
} else {
    win32: LIBS += -L"$$top_srcdir/thirdparty/openvr/lib/win32"
    else:macx: LIBS += -L"$$top_srcdir/thirdparty/openvr/lib/osx32"
    else:unix: LIBS += -L"$$top_srcdir/thirdparty/openvr/lib/linux32"
}

LIBS += -lopenvr_api

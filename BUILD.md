# Table of Contents
* [Windows](#windows)
  * [Building](#building)
  * [Packaging](#packaging)

# Windows
## Building
The [build_mingw](/scripts/build_mingw.bat) script allows you to compile the application using MinGW and copy the required files in a single step.

Install [Qt](https://www.qt.io/download) and its prebuilt MinGW components.

Open a terminal and run (replace variables accordingly):

```
set "PROJECT_ROOT=C:\Users\*username*\Downloads\OVRPhoneBridge"
set "QT_MINGW_PATH=C:\Qt\5.12.2\mingw73_64"
set "BUILD_ARCH=win64"
set "BUILD_TARGET=release"

%PROJECT_ROOT%\scripts\build_mingw.bat
```

In the case everything works, the output will be available in `build\win64\package_release`.

If you need to compile using MSVC, you will have to edit the build script or compile manually (make sure you copy required files alongside the application).

## Packaging
The [package](/scripts/package.bat) script will create a GZIP archive and an installer for the application.

This script expects to find files where the build script put them, do not move these files around before packaging.

If you want to create an installer, install [NSIS](https://nsis.sourceforge.io/Download), otherwise omit the `NSIS_PATH` variable later.

Open a terminal and run (replace variables accordingly):

```
set "PROJECT_ROOT=C:\Users\*username*\Downloads\OVRPhoneBridge"
set "NSIS_PATH=C:\Program Files (x86)\NSIS"
set "BUILD_ARCH=win64"
set "BUILD_TARGET=release"

%PROJECT_ROOT%\scripts\package.bat
```

You should now have an archive and an installer in `dist\release`.

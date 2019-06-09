# Table of Contents
* [Windows](#windows)
  * [Preliminary steps](#preliminary-steps)
  * [Building](#building)
  * [Packaging](#packaging)

# Windows
## Preliminary steps
Clone this repository and its submodules:

```
git clone --recursive https://github.com/Desuuuu/OVRPhoneBridge.git
```

If you wish to use the provided scripts to build and package the application, make sure the following tools are available on your system:

* `curl`
* `gpg`
* `tar`

Before starting the build process, you need to download or compile [libsodium](https://download.libsodium.org/doc/installation).

The [download_libsodium_mingw](/scripts/download_libsodium_mingw.bat) script can download and verify libsodium binaries for you.

Make sure you import [jedisct1's PGP key](https://download.libsodium.org/doc/installation#integrity-checking) into your GPG keyring then simply run the script.

If the script succeeds, you are now ready to build the application.

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

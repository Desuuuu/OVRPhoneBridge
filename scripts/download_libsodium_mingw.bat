@echo off

setlocal

if "%LIBSODIUM_VERSION%" == "" set LIBSODIUM_VERSION=1.0.18

cd ..\thirdparty\libsodium || goto :error

echo.
echo Downloading libsodium-%LIBSODIUM_VERSION%-mingw.tar.gz
echo.

curl --show-error --fail -O "https://download.libsodium.org/libsodium/releases/libsodium-%LIBSODIUM_VERSION%-mingw.tar.gz" || goto :error

echo.
echo Downloading libsodium-%LIBSODIUM_VERSION%-mingw.tar.gz.sig
echo.

curl --show-error --fail -O "https://download.libsodium.org/libsodium/releases/libsodium-%LIBSODIUM_VERSION%-mingw.tar.gz.sig" || goto :error

echo.
echo Verifying GPG signature
echo.

gpg --verify libsodium-%LIBSODIUM_VERSION%-mingw.tar.gz.sig libsodium-%LIBSODIUM_VERSION%-mingw.tar.gz || goto :error

echo.
echo Extracting
echo.

tar -xf libsodium-%LIBSODIUM_VERSION%-mingw.tar.gz || goto :error

endlocal

echo %cmdcmdline% | findstr /i /c:"%~nx0" >nul && pause

exit /b 0

:error

endlocal

echo %cmdcmdline% | findstr /i /c:"%~nx0" >nul && pause

exit /b 1

@echo off

setlocal

if not "%~f1" == "" set QT_SOURCE_PATH=%~f1
if not "%~2" == "" set QT_VERSION=%~2
if not "%~f3" == "" set MINGW_PATH=%~f3
if not "%~f4" == "" set OPENSSL_PATH=%~f4
if not "%~f5" == "" set D3D_PATH=%~f5
if not "%~6" == "" set BUILD_ARCH=%~6
if not "%~f7" == "" set INSTALL_PATH=%~f7

if not defined QT_SOURCE_PATH goto :source_path_invalid
if not exist "%QT_SOURCE_PATH%\configure.bat" goto :source_path_invalid

if not defined QT_VERSION goto :version_invalid
if "%QT_VERSION%" == "" goto :version_invalid

if not defined BUILD_ARCH goto :arch_invalid

if "%BUILD_ARCH%" == "win32" goto :arch_ok
if "%BUILD_ARCH%" == "win64" goto :arch_ok

goto :arch_invalid

:arch_ok

if not defined MINGW_PATH goto :mingw_path_invalid
if not exist "%MINGW_PATH%\bin\mingw32-make.exe" goto :mingw_path_invalid

if not defined OPENSSL_PATH goto :openssl_ok
if "%OPENSSL_PATH%" == "" goto :openssl_ok

if not exist "%OPENSSL_PATH%\include" goto :openssl_path_invalid

:openssl_ok

if not defined D3D_PATH goto :d3d_ok
if "%D3D_PATH%" == "" goto :d3d_ok

if not exist "%D3D_PATH%" goto :d3d_path_invalid

:d3d_ok

if not defined MYSQL_PATH goto :mysql_ok
if "%MYSQL_PATH%" == "" goto :mysql_ok

if not exist "%MYSQL_PATH%\include" goto :mysql_path_invalid
if not exist "%MYSQL_PATH%\lib" goto :mysql_path_invalid

:mysql_ok

rem Main script

set PATH=%QT_SOURCE_PATH%\qtbase\bin;%QT_SOURCE_PATH%\gnuwin32\bin;%QT_SOURCE_PATH%\qtrepotools\bin;%PATH%

set PATH=%MINGW_PATH%\bin;%PATH%

if not defined OPENSSL_PATH goto :openssl_path_set
if "%OPENSSL_PATH%" == "" goto :openssl_path_set

set PATH=%OPENSSL_PATH%\bin;%PATH%

:openssl_path_set

if not defined D3D_PATH goto :d3d_path_set
if "%D3D_PATH%" == "" goto :d3d_path_set

set PATH=%D3D_PATH%;%PATH%

:d3d_path_set

cd "%QT_SOURCE_PATH%"

if "%BUILD_CLEAN%" == "1" (
	echo Cleaning build
	echo.

  git submodule foreach --recursive "git clean -dfxq" || goto :error
  git clean -dfxq || goto :error
)

echo Initializing repository
echo.

git checkout %QT_VERSION% || goto :error
perl init-repository -q --module-subset=default,-qtwebengine || goto :error

echo Running configure
echo.

set CONFIGURE_ARGS=-opensource -confirm-license -release -opengl dynamic -nomake examples -nomake tests -skip qtwebengine -platform win32-g++ -qt-zlib -plugin-sql-sqlite -plugin-sql-odbc

if not defined INSTALL_PATH goto :no_prefix
if "%INSTALL_PATH%" == "" goto :no_prefix

set CONFIGURE_ARGS=%CONFIGURE_ARGS% -prefix "%INSTALL_PATH%"

:no_prefix

if not defined MYSQL_PATH goto :mysql_configured
if "%MYSQL_PATH%" == "" goto :mysql_configured

set CONFIGURE_ARGS=%CONFIGURE_ARGS% -plugin-sql-mysql -I "%MYSQL_PATH%\include" -L "%MYSQL_PATH%\lib"

:mysql_configured

if not defined OPENSSL_PATH goto :openssl_configured
if "%OPENSSL_PATH%" == "" goto :openssl_configured

set CONFIGURE_ARGS=%CONFIGURE_ARGS% -openssl-runtime OPENSSL_INCDIR="%OPENSSL_PATH%\include"

:openssl_configured

call configure.bat %CONFIGURE_ARGS% || goto :error

echo.
echo Running make
echo.

if not defined NCORE set NCORE=%NUMBER_OF_PROCESSORS%
if "%NCORE%" == "" set NCORE=%NUMBER_OF_PROCESSORS%

mingw32-make -j%NCORE% || goto :error

if not defined INSTALL_PATH goto :install_done
if "%INSTALL_PATH%" == "" goto :install_done

echo.
echo Running make install
echo.

mingw32-make -j%NCORE% install || goto :error

if exist "%INSTALL_PATH%\bin\qtenv2.bat" goto :qtenv_ok

echo.
echo Creating qtenv2.bat
echo.

(
  echo @echo off
  echo echo Setting up environment for Qt usage...
  echo set "PATH=%INSTALL_PATH%\bin;%MINGW_PATH%\bin;%%PATH%%"
  echo cd /D %INSTALL_PATH%
) > "%INSTALL_PATH%\bin\qtenv2.bat"

:qtenv_ok

if exist "%INSTALL_PATH%\bin\d3dcompiler_47.dll" goto :d3dcompiler_ok
if not "%DOWNLOAD_D3DCOMPILER%" == "1" goto :d3dcompiler_ok

cd "%INSTALL_PATH%\bin" || goto :d3dcompiler_ok

echo.
echo Downloading d3dcompiler_47.dll
echo.

if "%BUILD_ARCH%" == "win32" set "D3DCOMPILER_URL=https://download.qt.io/development_releases/prebuilt/d3dcompiler/msvc2013/d3dcompiler_47-x86.7z"
if "%BUILD_ARCH%" == "win64" set "D3DCOMPILER_URL=https://download.qt.io/development_releases/prebuilt/d3dcompiler/msvc2013/d3dcompiler_47-x64.7z"

curl --location --show-error --fail -o d3dcompiler.7z "%D3DCOMPILER_URL%" || goto :d3dcompiler_ok

7z e -y d3dcompiler.7z
del /f /q d3dcompiler.7z >nul 2>&1

:d3dcompiler_ok

if exist "%INSTALL_PATH%\bin\opengl32sw.dll" goto :opengl32sw_ok
if not "%DOWNLOAD_OPENGL32SW%" == "1" goto :opengl32sw_ok

cd "%INSTALL_PATH%\bin" || goto :opengl32sw_ok

echo.
echo Downloading opengl32sw.dll
echo.

if "%BUILD_ARCH%" == "win32" set "OPENGL32SW_URL=https://download.qt.io/development_releases/prebuilt/llvmpipe/windows/opengl32sw-32-mesa_12_0_rc2.7z"
if "%BUILD_ARCH%" == "win64" set "OPENGL32SW_URL=https://download.qt.io/development_releases/prebuilt/llvmpipe/windows/opengl32sw-64-mesa_12_0_rc2.7z"

curl --location --show-error --fail -o opengl32sw.7z "%OPENGL32SW_URL%" || goto :opengl32sw_ok

7z e -y opengl32sw.7z
del /f /q opengl32sw.7z >nul 2>&1

:opengl32sw_ok

:install_done

echo.
echo Built Qt %BUILD_ARCH% successfully!
echo.

endlocal

echo %cmdcmdline% | findstr /i /c:"%~nx0" >nul && pause

exit /b 0

rem Error cases

:source_path_invalid

echo Invalid QT_SOURCE_PATH: %QT_SOURCE_PATH% 1>&2
echo. 1>&2

goto :error

:version_invalid

echo Invalid QT_VERSION: %QT_VERSION% 1>&2
echo. 1>&2

goto :error

:arch_invalid

echo Invalid BUILD_ARCH: %BUILD_ARCH% 1>&2
echo. 1>&2

goto :error

:mingw_path_invalid

echo Invalid MINGW_PATH: %MINGW_PATH% 1>&2
echo. 1>&2

goto :error

:openssl_path_invalid

echo Invalid OPENSSL_PATH: %OPENSSL_PATH% 1>&2
echo. 1>&2

goto :error

:d3d_path_invalid

echo Invalid D3D_PATH: %D3D_PATH% 1>&2
echo. 1>&2

goto :error

:mysql_path_invalid

echo Invalid MYSQL_PATH: %MYSQL_PATH% 1>&2
echo. 1>&2

goto :error

:error

endlocal

echo %cmdcmdline% | findstr /i /c:"%~nx0" >nul && pause

exit /b 1

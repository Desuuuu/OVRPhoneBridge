@echo off

setlocal

if not "%~f1" == "" set PROJECT_ROOT=%~f1
if not "%~2" == "" set BUILD_ARCH=%~2
if not "%~f3" == "" set QT_MINGW_PATH=%~f3
if not "%~4" == "" set BUILD_TARGET=%~4

if not defined PROJECT_ROOT goto :project_root_invalid
if not exist "%PROJECT_ROOT%\OVRPhoneBridge.pro" goto :project_root_invalid

if not defined BUILD_ARCH goto :arch_invalid

if "%BUILD_ARCH%" == "win32" goto :arch_ok
if "%BUILD_ARCH%" == "win64" goto :arch_ok

goto :arch_invalid

:arch_ok

if not defined BUILD_TARGET set BUILD_TARGET=release

if "%BUILD_TARGET%" == "debug" goto :target_ok
if "%BUILD_TARGET%" == "release" goto :target_ok

goto :target_invalid

:target_ok

if not defined QT_MINGW_PATH goto :mingw_path_invalid
if not exist "%QT_MINGW_PATH%\bin\qtenv2.bat" goto :mingw_path_invalid

rem Main script

call "%QT_MINGW_PATH%\bin\qtenv2.bat" || goto :error
echo.

cd "%PROJECT_ROOT%"

if "%BUILD_CLEAN%" == "1" (
	echo Cleaning `build\%BUILD_ARCH%` folder
	echo.

	rmdir /s /q build\%BUILD_ARCH% >nul 2>&1
)

echo Creating `build\%BUILD_ARCH%` folder
echo.

mkdir build\%BUILD_ARCH% >nul 2>&1
cd build\%BUILD_ARCH% || goto :error

echo Running qmake
echo.

qmake "%PROJECT_ROOT%\OVRPhoneBridge.pro" -spec win32-g++ "CONFIG+=%BUILD_TARGET%" || goto :error

echo.
echo Running make
echo.

if not defined NCORE set NCORE=%NUMBER_OF_PROCESSORS%
if "%NCORE%" == "" set NCORE=%NUMBER_OF_PROCESSORS%

mingw32-make -j%NCORE% || goto :error

set PACKAGE_DIR=package_%BUILD_TARGET%

echo.
echo Creating `build\%BUILD_ARCH%\%PACKAGE_DIR%` folder
echo.

rmdir /s /q %PACKAGE_DIR% >nul 2>&1
mkdir %PACKAGE_DIR% || goto :error

echo Copying files to `build\%BUILD_ARCH%\%PACKAGE_DIR%` folder
echo.

xcopy /e /y %BUILD_TARGET%\OVRPhoneBridge.exe %PACKAGE_DIR% || goto :error
xcopy /e /y "%PROJECT_ROOT%\bundle\all\*" %PACKAGE_DIR% || goto :error
xcopy /e /y "%PROJECT_ROOT%\bundle\windows\*" %PACKAGE_DIR% || goto :error
xcopy /e /y "%PROJECT_ROOT%\thirdparty\libsodium\libsodium-%BUILD_ARCH%\bin\*.dll" %PACKAGE_DIR% || goto :error
xcopy /e /y "%PROJECT_ROOT%\thirdparty\openvr\bin\%BUILD_ARCH%\*.dll" %PACKAGE_DIR% || goto :error

echo.
echo Running windeployqt
echo.

cd %PACKAGE_DIR% || goto :error
windeployqt --plugindir plugins --%BUILD_TARGET% --no-translations --no-system-d3d-compiler --no-opengl-sw OVRPhoneBridge.exe || goto :error

echo.
echo Built %BUILD_ARCH%/%BUILD_TARGET% successfully!
echo.

endlocal

echo %cmdcmdline% | findstr /i /c:"%~nx0" >nul && pause

exit /b 0

rem Error cases

:project_root_invalid

echo Invalid PROJECT_ROOT: %PROJECT_ROOT% 1>&2
echo. 1>&2

goto :error

:arch_invalid

echo Invalid BUILD_ARCH: %BUILD_ARCH% 1>&2
echo. 1>&2

goto :error

:target_invalid

echo Invalid BUILD_TARGET: %BUILD_TARGET% 1>&2
echo. 1>&2

goto :error

:mingw_path_invalid

echo Invalid QT_MINGW_PATH: %QT_MINGW_PATH% 1>&2
echo. 1>&2

goto :error

:error

endlocal

echo %cmdcmdline% | findstr /i /c:"%~nx0" >nul && pause

exit /b 1

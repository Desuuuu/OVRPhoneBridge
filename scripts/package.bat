@echo off

setlocal

if not "%~f1" == "" set PROJECT_ROOT=%~f1
if not "%~2" == "" set BUILD_ARCH=%~2
if not "%~f3" == "" set NSIS_PATH=%~f3
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

rem Main script

set BUILD_PATH=%PROJECT_ROOT%\build\%BUILD_ARCH%

if not exist %BUILD_PATH%\version.txt goto :build_not_found
if not exist %BUILD_PATH%\package_%BUILD_TARGET% goto :build_not_found

set /p BUILD_VERSION=< %BUILD_PATH%\version.txt

if not defined BUILD_VERSION goto :build_not_found
if "%BUILD_VERSION%" == "" goto :build_not_found

echo Build version: %BUILD_VERSION%
echo.

cd "%PROJECT_ROOT%"

echo Creating `dist` folder
echo.

mkdir dist >nul 2>&1
cd dist || goto :error

echo Copying build files to `dist\OVRPhoneBridge`
echo.

rmdir /s /q OVRPhoneBridge >nul 2>&1
xcopy /e /y %BUILD_PATH%\package_%BUILD_TARGET% OVRPhoneBridge\* || goto :error

echo Creating `dist\%BUILD_TARGET%` folder
echo.

mkdir %BUILD_TARGET% >nul 2>&1

echo Creating `OVRPhoneBridge_%BUILD_VERSION%_%BUILD_ARCH%.tar.gz` archive
echo.

tar -czf %BUILD_TARGET%\OVRPhoneBridge_%BUILD_VERSION%_%BUILD_ARCH%.tar.gz OVRPhoneBridge || goto :error

if not defined NSIS_PATH (
	echo NSIS_PATH not set, skipping installer creation
	echo.

	goto :done
)

if not exist "%NSIS_PATH%" goto :nsis_not_found

echo Generating `uninstall.nsh` script
echo.

call :generate_uninstall OVRPhoneBridge > uninstall.nsh || goto :error

set PATH=%NSIS_PATH%;%PATH%

set NSIS_ARGS=/DSRC_DIR=OVRPhoneBridge
set NSIS_ARGS=%NSIS_ARGS% /DOUTPUT_FILE=%BUILD_TARGET%\OVRPhoneBridge_installer_%BUILD_VERSION%_%BUILD_ARCH%.exe
set NSIS_ARGS=%NSIS_ARGS% /DEXE_NAME=OVRPhoneBridge.exe
set NSIS_ARGS=%NSIS_ARGS% /DICON_PATH=icon.ico
set NSIS_ARGS=%NSIS_ARGS% /DLICENSE_PATH=LICENSE
set NSIS_ARGS=%NSIS_ARGS% /DBUILD_VERSION=%BUILD_VERSION%

if "%BUILD_ARCH%" == "win64" set NSIS_ARGS=%NSIS_ARGS% /DBUILD_64

echo Running makensis
echo.

makensis /V2 /WX /NOCD %NSIS_ARGS% "%PROJECT_ROOT%\scripts\installer.nsi" || goto :error

:done
rmdir /s /q "%PROJECT_ROOT%\dist\OVRPhoneBridge" >nul 2>&1
del /f /q "%PROJECT_ROOT%\dist\uninstall.nsh" >nul 2>&1

endlocal

echo %cmdcmdline% | findstr /i /c:"%~nx0" >nul && pause

exit /b 0

rem Subroutines

:generate_uninstall
if "%~f1" == "" (
	echo Missing directory 1>&2
	exit /b 1
)

if not exist "%~f1" (
	echo Invalid directory 1>&2
	exit /b 1
)

setlocal EnableDelayedExpansion

set I=0
set LOCATION=%~f1

for /f "delims=" %%d in ('dir /b /s /ad %LOCATION%') do (
	set /a I=!I! + 1

	set FILE_PATH=%%d
	set FILE_PATH=!FILE_PATH:%LOCATION%\=!

	set LINE!I!=RMDir $INSTDIR\!FILE_PATH!
)

for /f "delims=" %%f in ('dir /b /s /a-d %LOCATION%') do (
	set /a I=!I! + 1

	set FILE_PATH=%%f
	set FILE_PATH=!FILE_PATH:%LOCATION%\=!

	set LINE!I!=Delete $INSTDIR\!FILE_PATH!
)

for /L %%l in (!I!,-1,1) do (
	echo !LINE%%l!
)

endlocal

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

:build_not_found

echo Build files not found: %BUILD_PATH% 1>&2
echo. 1>&2

goto :error

:nsis_not_found

echo NSIS not found: %NSIS_PATH% 1>&2
echo. 1>&2

goto :error

:error

if exist "%PROJECT_ROOT%\OVRPhoneBridge.pro" (
	rmdir /s /q "%PROJECT_ROOT%\dist\OVRPhoneBridge" >nul 2>&1
	del /f /q "%PROJECT_ROOT%\dist\uninstall.nsh" >nul 2>&1
)

endlocal

echo %cmdcmdline% | findstr /i /c:"%~nx0" >nul && pause

exit /b 1

!include "MUI2.nsh"

!define APP_NAME "OVRPhoneBridge"

!define UPDATE_URL "https://github.com/Desuuuu/OVRPhoneBridge/releases"

RequestExecutionLevel admin

!ifndef SRC_DIR
	!error "SRC_DIR is not defined"
!endif

!ifndef OUTPUT_FILE
	!error "OUTPUT_FILE is not defined"
!endif

!ifndef EXE_NAME
	!error "EXE_NAME is not defined"
!endif

!ifndef LICENSE_PATH
	!error "LICENSE_PATH is not defined"
!endif

!ifndef BUILD_VERSION
	!error "BUILD_VERSION is not defined"
!endif

!ifdef BUILD_64
	InstallDir "$PROGRAMFILES64\OVRPhoneBridge"
!else
	InstallDir "$PROGRAMFILES\OVRPhoneBridge"
!endif

InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"InstallLocation"

!define MUI_ABORTWARNING

!define MUI_ICON "${SRC_DIR}\icon.ico"

Name "${APP_NAME}"
OutFile "${OUTPUT_FILE}"

!insertmacro MUI_PAGE_LICENSE "${LICENSE_PATH}"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

!macro KillProcess
	DetailPrint "Stopping OVRPhoneBridge process..."

	ExecWait "taskkill /f /im ${EXE_NAME}"

	Sleep 2000
!macroend

Var IS_UPGRADE
Var UNINSTALL_PURGE

Function .onInit
	StrCpy $IS_UPGRADE "0"

	ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
			"UninstallString"

	StrCmp $0 "" initOk

	MessageBox MB_YESNO|MB_ICONINFORMATION \
			"${APP_NAME} is already installed.$\n$\nWould you like to upgrade your current installation?" \
			/SD IDYES \
			IDYES upgrade

	Abort

	upgrade:
	StrCpy $IS_UPGRADE "1"

	initOk:
FunctionEnd

Function .onInstSuccess
	Exec '"$INSTDIR\${EXE_NAME}" --silent'
FunctionEnd

Section "Install"
	!insertmacro KillProcess

	StrCmp $IS_UPGRADE "1" 0 +3
		DetailPrint "Uninstalling previous version..."
		ExecWait '"$INSTDIR\uninstall.exe" /S _?=$INSTDIR'

	SetOutPath $INSTDIR

	File /r ${SRC_DIR}\*.*

	DetailPrint "Installing VR manifest..."

	ClearErrors
	ExecWait '"$INSTDIR\${EXE_NAME}" --silent --install_manifest'

	IfErrors 0 +2
		DetailPrint "Failed to install VR manifest"

	WriteUninstaller "$INSTDIR\uninstall.exe"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"DisplayName" "${APP_NAME}"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"UninstallString" "$\"$INSTDIR\uninstall.exe$\""

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"InstallLocation" "$\"$INSTDIR$\""

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"DisplayIcon" "$\"$INSTDIR\icon.ico$\""

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"URLUpdateInfo" "${UPDATE_URL}"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"DisplayVersion" "${BUILD_VERSION}"

	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
			"NoModify" 1

	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
			"NoRepair" 1
SectionEnd

Function un.onInit
	StrCpy $UNINSTALL_PURGE "0"

	MessageBox MB_YESNO|MB_ICONINFORMATION \
			"Do you want to remove settings and logs?" \
			/SD IDNO \
			IDNO initOk

	StrCpy $UNINSTALL_PURGE "1"

	initOk:
FunctionEnd

Section "un.Uninstall"
	!insertmacro KillProcess

	DetailPrint "Uninstalling VR manifest..."

	ClearErrors
	ExecWait '"$INSTDIR\${EXE_NAME}" --silent --uninstall_manifest'

	IfErrors 0 +2
		DetailPrint "Failed to uninstall VR manifest"

	!include uninstall.nsh

	SetShellVarContext current

	StrCmp $UNINSTALL_PURGE "1" 0 +4
		Delete $APPDATA\OVRPhoneBridge\settings.ini
		Delete $APPDATA\OVRPhoneBridge\OVRPhoneBridge.log
		RMDir $APPDATA\OVRPhoneBridge

	Delete $INSTDIR\uninstall.exe

	RMDir $INSTDIR

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
SectionEnd

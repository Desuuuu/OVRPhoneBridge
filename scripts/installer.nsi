!include "MUI2.nsh"
!include "nsProcess.nsh"

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

!ifndef ICON_PATH
	!error "ICON_PATH is not defined"
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

!define MUI_ICON "${SRC_DIR}\${ICON_PATH}"

!define MUI_ABORTWARNING

!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_SHOWREADME $APPDATA\OVRPhoneBridge\identifier.txt
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Show identifier"

!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "StartMenuFolder"

Name "${APP_NAME}"
OutFile "${OUTPUT_FILE}"

Var StartMenuFolder

!insertmacro MUI_PAGE_LICENSE "${SRC_DIR}\${LICENSE_PATH}"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

!macro KillProcess
	nsProcess::_FindProcess "${EXE_NAME}"
	Pop $0
	IntCmp $0 0 0 +4 +4
		DetailPrint "Stopping ${EXE_NAME}"
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
	nsProcess::_FindProcess "vrcompositor.exe"
	Pop $0
	IntCmp $0 0 0 +2 +2
		Exec '"$INSTDIR\${EXE_NAME}" --silent'
FunctionEnd

Section "Install"
	!insertmacro KillProcess

	StrCmp $IS_UPGRADE "1" 0 +3
		DetailPrint "Uninstalling previous version..."
		ExecWait '"$INSTDIR\uninstall.exe" /S _?=$INSTDIR'

	SetOutPath $INSTDIR

	File /r ${SRC_DIR}\*.*

	DetailPrint "Generating key pair..."

	ClearErrors
	ExecWait '"$INSTDIR\${EXE_NAME}" --silent --generate_keypair'

	IfErrors 0 +2
		DetailPrint "Failed to generate key pair"

	DetailPrint "Installing VR manifest..."

	ClearErrors
	ExecWait '"$INSTDIR\${EXE_NAME}" --silent --install_manifest'

	IfErrors 0 +2
		DetailPrint "Failed to install VR manifest"

	WriteUninstaller "$INSTDIR\uninstall.exe"

	SetShellVarContext all

	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application

		CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${APP_NAME}.lnk" "$INSTDIR\${EXE_NAME}"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\uninstall.exe"

	!insertmacro MUI_STARTMENU_WRITE_END

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"DisplayName" "${APP_NAME}"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"UninstallString" "$\"$INSTDIR\uninstall.exe$\""

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"InstallLocation" "$\"$INSTDIR$\""

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" \
		"DisplayIcon" "$\"$INSTDIR\${ICON_PATH}$\""

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

	SetShellVarContext all

	!insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder

	IfFileExists "$SMPROGRAMS\$StartMenuFolder" 0 +4
		Delete "$SMPROGRAMS\$StartMenuFolder\${APP_NAME}.lnk"
		Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
		RMDir "$SMPROGRAMS\$StartMenuFolder"

	!include uninstall.nsh

	SetShellVarContext current

	StrCmp $UNINSTALL_PURGE "1" 0 +5
		Delete $APPDATA\OVRPhoneBridge\settings.ini
		Delete $APPDATA\OVRPhoneBridge\identifier.txt
		Delete $APPDATA\OVRPhoneBridge\OVRPhoneBridge.log
		RMDir $APPDATA\OVRPhoneBridge

	Delete $INSTDIR\uninstall.exe

	RMDir $INSTDIR

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
SectionEnd

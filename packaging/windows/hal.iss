; Inno Setup script of the HAL installer. Build it with tools\windows\package.ps1, which passes the definitions below.
#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef SourceDir
  #error SourceDir (the packaged HAL folder) is not defined
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif
#ifndef IconFile
  #define IconFile "..\..\app\hal.ico"
#endif

[Setup]
AppId={{7C1E5B2A-3D4F-4E8A-9F61-2B5D8A0C9E13}
AppName=HAL
AppVersion={#AppVersion}
AppVerName=HAL {#AppVersion}
AppPublisher=hal-windows
AppPublisherURL=https://github.com/horhulenko/hal-windows
DefaultDirName={autopf}\HAL
DefaultGroupName=HAL
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename=HALSetup
SetupIconFile={#IconFile}
UninstallDisplayIcon={app}\bin\hal-gui.exe
LicenseFile={#SourceDir}\LICENSE.txt
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; installs for the current user without administrator rights, the wizard offers an all-users install as well
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{autoprograms}\HAL"; Filename: "{app}\bin\hal-gui.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\HAL"; Filename: "{app}\bin\hal-gui.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\hal-gui.exe"; Description: "Start HAL"; Flags: nowait postinstall skipifsilent

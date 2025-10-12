; Kolosal Code Inno Setup Script
; Build on Windows with: iscc installer/kolosal.iss

[Setup]
AppName=Kolosal Code
AppVersion=0.0.14
AppPublisher=KolosalAI
DefaultDirName={pf64}\KolosalAI\Kolosal Code
DefaultGroupName=KolosalAI
OutputBaseFilename=KolosalCodeSetup
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
WizardStyle=modern

[Files]
; Expects dist/win/kolosal.exe to exist (built via `npm run build:win:exe`)
Source: "dist\\win\\kolosal.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\\Kolosal Code"; Filename: "{app}\\kolosal.exe"

[Run]
Filename: "{app}\\kolosal.exe"; Description: "Run Kolosal from installer"; Flags: nowait postinstall skipifsilent

[Tasks]
Name: desktopicon; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; Flags: unchecked

[Icons]
Name: "{commondesktop}\\Kolosal Code"; Filename: "{app}\\kolosal.exe"; Tasks: desktopicon

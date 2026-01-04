; aab2apk Windows Installer Script
; Inno Setup Script for aab2apk CLI tool
; Professional installer with PATH integration

#define MyAppName "aab2apk"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "aab2apk Contributors"
#define MyAppURL "https://github.com/blaziumdev/aab2apk"
#define MyAppExeName "aab2apk.exe"

[Setup]
; Basic installer information
AppId={{A1B2C3D4-E5F6-4A7B-8C9D-0E1F2A3B4C5D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

; Default installation directory (Program Files)
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes

; License file
LicenseFile=LICENSE

; Output directory and filename
OutputDir=installer
OutputBaseFilename=aab2apk-setup-{#MyAppVersion}

; Compression settings
Compression=lzma
SolidCompression=yes
WizardStyle=modern

; Privileges required (needed for Program Files and PATH modification)
PrivilegesRequired=admin

; Uninstall settings
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}

; Show language selection (optional)
; ShowLanguageDialog=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 6.1

[Files]
; Install the main executable
Source: "build\Release\aab2apk.exe"; DestDir: "{app}"; Flags: ignoreversion

; Install LICENSE file
Source: "LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
; Optional: Run after installation (we don't run the CLI during install)
; Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
// Custom code for PATH manipulation
const
    EnvironmentKey = 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';

// Function to safely add directory to PATH
procedure EnvAddPath(Path: string);
var
    Paths: string;
begin
    // Retrieve current PATH
    if not RegQueryStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Paths) then
    begin
        // PATH doesn't exist, create it
        RegWriteStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Path);
    end
    else
    begin
        // Check if the path is already in PATH
        if Pos(';' + Path + ';', ';' + Paths + ';') > 0 then
            exit; // Already in PATH, do nothing
        
        // Check if it's at the beginning
        if Pos(Path + ';', Paths) = 1 then
            exit; // Already at the beginning
        
        // Append to PATH
        Paths := Paths + ';' + Path;
        RegWriteStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Paths);
    end;
    // PATH changes are automatically picked up by new processes
end;

// Function to safely remove directory from PATH
procedure EnvRemovePath(Path: string);
var
    Paths: string;
begin
    // Skip if PATH doesn't exist
    if not RegQueryStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Paths) then
        exit;
    
    // Remove all occurrences (handle multiple if somehow added)
    StringChange(Paths, Path + ';', '');
    StringChange(Paths, ';' + Path, '');
    StringChange(Paths, Path, '');
    
    // Remove leading/trailing semicolons
    while (Length(Paths) > 0) and (Paths[1] = ';') do
        Delete(Paths, 1, 1);
    while (Length(Paths) > 0) and (Paths[Length(Paths)] = ';') do
        Delete(Paths, Length(Paths), 1);
    
    // Write back the modified PATH
    RegWriteStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Paths);
    // PATH changes are automatically picked up by new processes
end;

// Called during installation (after files are installed)
procedure CurStepChanged(CurStep: TSetupStep);
begin
    if CurStep = ssPostInstall then
    begin
        // Add installation directory to PATH
        EnvAddPath(ExpandConstant('{app}'));
    end;
end;

// Called during uninstallation
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
    if CurUninstallStep = usUninstall then
    begin
        // Remove installation directory from PATH
        EnvRemovePath(ExpandConstant('{app}'));
    end;
end;


#ifndef SourceRoot
  #define SourceRoot "..\artifacts\installer-stage\Release\x64"
#endif
#ifndef OutputRoot
  #define OutputRoot "output"
#endif
#ifndef ProductVersion
  #define ProductVersion "0.1.2"
#endif

#define ProductName "Menu11 for Git"
#define ProductExe "Menu11.exe"
#define ProductAppId "{0AABBA87-94C3-4B8D-AC55-E260F9EC8D5E}"

[Setup]
AppId={{#ProductAppId}
AppName={#ProductName}
AppVerName={#ProductName} {#ProductVersion}
AppVersion={#ProductVersion}
AppPublisher=Menu11 for Git contributors
AppComments=Native Windows 11 context menu integration for Git for Windows.
AppCopyright=Copyright (c) 2026 Menu11 for Git contributors
DefaultDirName={localappdata}\Programs\Menu11 for Git
DisableDirPage=yes
UsePreviousAppDir=no
AllowNetworkDrive=no
AllowUNCPath=no
DefaultGroupName=Menu11 for Git
DisableProgramGroupPage=auto
AllowNoIcons=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.22000
PrivilegesRequired=lowest
OutputDir={#OutputRoot}
OutputBaseFilename=Menu11ForGitSetup-{#ProductVersion}-x64
SetupIconFile=..\assets\AppIcon.ico
UninstallDisplayIcon={app}\Assets\AppIcon.ico
UninstallDisplayName=Menu11 for Git
VersionInfoVersion={#ProductVersion}.0
VersionInfoProductName=Menu11 for Git
VersionInfoDescription=Menu11 for Git Setup
VersionInfoCompany=Menu11 for Git contributors
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern dynamic windows11 hidebevels includetitlebar
WizardSizePercent=100
WizardKeepAspectRatio=yes
WizardBackColor=#FFFBF9
WizardBackColorDynamicDark=#321D18
WizardImageFile=..\assets\InstallerBanner.png
WizardImageFileDynamicDark=..\assets\InstallerBanner.png
WizardImageBackColor=#FFFBF9
WizardImageBackColorDynamicDark=#321D18
WizardImageStretch=yes
WizardImageOpacity=255
WizardSmallImageFile=..\assets\InstallerLogo.png
WizardSmallImageFileDynamicDark=..\assets\InstallerLogo.png
WizardSmallImageBackColor=none
WizardSmallImageBackColorDynamicDark=none
LicenseFile=..\LICENSE
CloseApplications=yes
RestartApplications=no
SetupLogging=yes
ChangesAssociations=yes
ShowLanguageDialog=yes
LanguageDetectionMethod=uilanguage
UsePreviousLanguage=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimplified"; MessagesFile: "compiler:Default.isl,ChineseSimplified.isl"

[CustomMessages]
english.WelcomeTitle=Modern Windows 11 Git integration
english.WelcomeBody=Menu11 adds lightweight, configurable Git commands directly to the native Windows 11 context menu.%n%nNo background service.%nNo startup process.%nUses your existing Git for Windows installation.
english.GitDetected=Git for Windows detected
english.GitRequired=Git for Windows is required
english.GitMissing=Menu11 for Git requires an existing Git for Windows installation. Setup cannot continue until Git for Windows is installed.
english.GitInstallRoot=Location: %1
english.GitVersion=Version: %1
english.FullInstallation=Full installation
english.InstallOptions=Installation options
english.InstallOptionsDescription=Choose the shortcuts and Windows integration to install.
english.RegisterContextMenu=Register the Windows 11 context menu extension
english.StartMenuShortcut=Add Menu11 for Git to the Start Menu
english.DesktopShortcut=Create a desktop shortcut
english.FinishedReady=Menu11 for Git is ready.%n%nWindows 11 context menu integration has been installed successfully.
english.LaunchMenu11=Launch Menu11 for Git
english.UnsupportedInstallLocation=Menu11 for Git must be installed in the current user's local Programs folder:%n%n%1%n%nCustom install locations are not supported by the Windows 11 context menu package registration.
english.RegistrationFailed=Windows 11 context menu registration failed. Setup did not complete successfully.%n%nDiagnostic details were saved to:%n%1%n%nRemove Menu11 for Git from Installed apps before retrying.
english.UnregistrationFailed=Menu11 for Git could not remove its package registration. Close Explorer windows and try uninstalling again.

chinesesimplified.WelcomeTitle=原生 Windows 11 Git 集成
chinesesimplified.WelcomeBody=Menu11 将轻量、可配置的 Git 命令直接加入 Windows 11 原生现代右键菜单。%n%n无后台服务。%n无开机启动进程。%n复用本机现有的 Git for Windows。
chinesesimplified.GitDetected=已检测到 Git for Windows
chinesesimplified.GitRequired=需要 Git for Windows
chinesesimplified.GitMissing=Menu11 for Git 需要本机已安装 Git for Windows。安装 Git for Windows 之前，安装程序无法继续。
chinesesimplified.GitInstallRoot=位置：%1
chinesesimplified.GitVersion=版本：%1
chinesesimplified.FullInstallation=完整安装
chinesesimplified.InstallOptions=安装选项
chinesesimplified.InstallOptionsDescription=选择要安装的快捷方式和 Windows 集成功能。
chinesesimplified.RegisterContextMenu=注册 Windows 11 右键菜单扩展
chinesesimplified.StartMenuShortcut=将 Menu11 for Git 添加到开始菜单
chinesesimplified.DesktopShortcut=创建桌面快捷方式
chinesesimplified.FinishedReady=Menu11 for Git 已准备就绪。%n%nWindows 11 右键菜单集成已成功安装。
chinesesimplified.LaunchMenu11=启动 Menu11 for Git
chinesesimplified.UnsupportedInstallLocation=Menu11 for Git 必须安装到当前用户的本地 Programs 目录：%n%n%1%n%nWindows 11 右键菜单包注册不支持自定义安装位置。
chinesesimplified.RegistrationFailed=Windows 11 右键菜单注册失败，安装未成功完成。%n%n诊断详情已保存到：%n%1%n%n重试前，请先从“已安装的应用”中移除 Menu11 for Git。
chinesesimplified.UnregistrationFailed=Menu11 for Git 无法移除包注册。请关闭资源管理器窗口后重试卸载。

[Types]
Name: "full"; Description: "{cm:FullInstallation}"; Flags: iscustom

[Components]
Name: "contextmenu"; Description: "{cm:RegisterContextMenu}"; Types: full; Flags: fixed

[Tasks]
Name: "startmenu"; Description: "{cm:StartMenuShortcut}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: checkedonce
Name: "desktopicon"; Description: "{cm:DesktopShortcut}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceRoot}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "prepare-upgrade.ps1"; Flags: dontcopy

[Registry]
Root: HKCU; Subkey: "Software\Menu11ForGit"; Flags: uninsdeletekey

[Icons]
Name: "{group}\Menu11 for Git"; Filename: "{app}\{#ProductExe}"; WorkingDir: "{app}"; IconFilename: "{app}\Assets\AppIcon.ico"; Tasks: startmenu
Name: "{autodesktop}\Menu11 for Git"; Filename: "{app}\{#ProductExe}"; WorkingDir: "{app}"; IconFilename: "{app}\Assets\AppIcon.ico"; Tasks: desktopicon

[Run]
Filename: "{sysnative}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -NonInteractive -ExecutionPolicy Bypass -File ""{app}\Installer\register-package.ps1"" -InstallDirectory ""{app}"""; StatusMsg: "{cm:RegisterContextMenu}"; Flags: runhidden waituntilterminated; Components: contextmenu; AfterInstall: VerifyPackageRegistration
Filename: "{app}\{#ProductExe}"; Description: "{cm:LaunchMenu11}"; Flags: nowait postinstall skipifsilent unchecked; Check: PackageRegistrationSucceeded

[UninstallRun]
Filename: "{sysnative}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -NonInteractive -ExecutionPolicy Bypass -File ""{app}\Installer\unregister-package.ps1"" -InstallDirectory ""{app}"""; Flags: runhidden waituntilterminated; RunOnceId: "UnregisterMenu11Package"

[Code]
var
  GitRoot: String;
  GitVersionText: String;
  GitStatusPage: TOutputMsgWizardPage;
  PackageRegistrationFailed: Boolean;

function FileExistsAtGitRoot(const Root, RelativePath: String): Boolean;
begin
  Result := (Root <> '') and FileExists(AddBackslash(Root) + RelativePath);
end;

function GitExeAtRoot(const Root: String): String;
begin
  if FileExistsAtGitRoot(Root, 'cmd\git.exe') then
    Result := AddBackslash(Root) + 'cmd\git.exe'
  else if FileExistsAtGitRoot(Root, 'bin\git.exe') then
    Result := AddBackslash(Root) + 'bin\git.exe'
  else
    Result := '';
end;

function NormalizeGitRoot(const Candidate: String): String;
var
  Root: String;
begin
  Root := RemoveBackslashUnlessRoot(Candidate);
  if FileExistsAtGitRoot(Root, 'cmd\git.exe') or
     FileExistsAtGitRoot(Root, 'bin\git.exe') then
    Result := Root
  else
    Result := '';
end;

function TryGitRootFromRegistry(const RootKey: Integer; const Subkey: String; var Root: String): Boolean;
var
  Value: String;
begin
  Result := False;
  if RegQueryStringValue(RootKey, Subkey, 'InstallPath', Value) then
  begin
    Root := NormalizeGitRoot(Value);
    Result := Root <> '';
  end;
end;

function TryGitRootFromGitExe(const GitExe: String; var Root: String): Boolean;
var
  Candidate: String;
begin
  Result := False;
  Candidate := ExtractFileDir(ExtractFileDir(GitExe));
  Root := NormalizeGitRoot(Candidate);
  Result := Root <> '';
end;

function DetectGitForWindows: Boolean;
var
  ResultCode: Integer;
  TempFile: String;
  Lines: TArrayOfString;
  CommonRoot: String;
  GitExe: String;
begin
  GitRoot := '';
  GitVersionText := '';

  Result :=
    TryGitRootFromRegistry(HKLM64, 'SOFTWARE\GitForWindows', GitRoot) or
    TryGitRootFromRegistry(HKLM32, 'SOFTWARE\GitForWindows', GitRoot) or
    TryGitRootFromRegistry(HKCU64, 'SOFTWARE\GitForWindows', GitRoot) or
    TryGitRootFromRegistry(HKCU32, 'SOFTWARE\GitForWindows', GitRoot);

  if not Result then
  begin
    CommonRoot := ExpandConstant('{autopf}\Git');
    GitRoot := NormalizeGitRoot(CommonRoot);
    Result := GitRoot <> '';
  end;

  if not Result then
  begin
    TempFile := ExpandConstant('{tmp}\menu11-git-location.txt');
    if Exec(ExpandConstant('{cmd}'), '/D /C where git.exe > "' + TempFile + '"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) and
       (ResultCode = 0) and LoadStringsFromFile(TempFile, Lines) and (GetArrayLength(Lines) > 0) then
      Result := TryGitRootFromGitExe(Trim(Lines[0]), GitRoot);
    DeleteFile(TempFile);
  end;

  if Result then
  begin
    GitExe := GitExeAtRoot(GitRoot);
    TempFile := ExpandConstant('{tmp}\menu11-git-version.txt');
    if (GitExe <> '') and
       Exec(
         ExpandConstant('{cmd}'),
         '/D /C ""' + GitExe + '" --version > "' + TempFile + '""',
         '', SW_HIDE, ewWaitUntilTerminated, ResultCode) and
       (ResultCode = 0) then
    begin
      if LoadStringsFromFile(TempFile, Lines) and (GetArrayLength(Lines) > 0) then
        GitVersionText := Trim(Lines[0]);
    end;
    DeleteFile(TempFile);
  end;
end;

function InitializeSetup: Boolean;
begin
  Result := DetectGitForWindows;
  if not Result then
    MsgBox(
      CustomMessage('GitRequired') + #13#10#13#10 + CustomMessage('GitMissing'),
      mbCriticalError, MB_OK);
end;

procedure InitializeWizard;
begin
  WizardForm.DirEdit.Text := ExpandConstant('{localappdata}\Programs\Menu11 for Git');
  WizardForm.WelcomeLabel1.Caption := CustomMessage('WelcomeTitle');
  WizardForm.WelcomeLabel2.Caption := CustomMessage('WelcomeBody');
  WizardForm.FinishedHeadingLabel.Caption := CustomMessage('FinishedReady');

  GitStatusPage := CreateOutputMsgPage(
    wpWelcome,
    CustomMessage('GitDetected'),
    FmtMessage(CustomMessage('GitInstallRoot'), [GitRoot]),
    FmtMessage(CustomMessage('GitVersion'), [GitVersionText]));
end;

procedure VerifyPackageRegistration;
var
  ResultCode: Integer;
  ErrorLogPath: String;
begin
  if not Exec(
    ExpandConstant('{sysnative}\WindowsPowerShell\v1.0\powershell.exe'),
    '-NoProfile -NonInteractive -Command "if (-not (Get-AppxPackage -Name Menu11ForGit.GitBash -ErrorAction SilentlyContinue)) { exit 1 }; if (-not (Get-AppxPackage -Name Menu11ForGit.GitGui -ErrorAction SilentlyContinue)) { exit 1 }; if (-not (Get-AppxPackage -Name Menu11ForGit.GitCommands -ErrorAction SilentlyContinue)) { exit 1 }"',
    '', SW_HIDE, ewWaitUntilTerminated, ResultCode) or (ResultCode <> 0) then
  begin
    PackageRegistrationFailed := True;
    ErrorLogPath := ExpandConstant('{app}\Installer\registration-error.log');
    SuppressibleMsgBox(
      FmtMessage(CustomMessage('RegistrationFailed'), [ErrorLogPath]),
      mbCriticalError, MB_OK, IDOK);
  end;
end;

function PackageRegistrationSucceeded: Boolean;
begin
  Result := not PackageRegistrationFailed;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := PackageRegistrationFailed and (PageID = wpFinished);
end;

function GetCustomSetupExitCode: Integer;
begin
  if PackageRegistrationFailed then
    Result := 10
  else
    Result := 0;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  if CompareText(
    AddBackslash(ExpandFileName(ExpandConstant('{app}'))),
    AddBackslash(ExpandFileName(ExpandConstant('{localappdata}\Programs\Menu11 for Git')))) <> 0 then
    Result := FmtMessage(CustomMessage('UnsupportedInstallLocation'), [ExpandConstant('{localappdata}\Programs\Menu11 for Git')])
  else if not DetectGitForWindows then
    Result := CustomMessage('GitMissing')
  else
  begin
    ExtractTemporaryFile('prepare-upgrade.ps1');
    if not Exec(
      ExpandConstant('{sysnative}\WindowsPowerShell\v1.0\powershell.exe'),
      '-NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
        ExpandConstant('{tmp}\prepare-upgrade.ps1') + '" -InstallDirectory "' +
        ExpandConstant('{app}') + '"',
      '', SW_HIDE, ewWaitUntilTerminated, ResultCode) or (ResultCode <> 0) then
      Result := CustomMessage('UnregistrationFailed')
    else
      Result := '';
  end;
end;

function InitializeUninstall: Boolean;
begin
  Result := True;
end;

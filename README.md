<div align="center">

  <img src="assets/AppIcon-256.png" width="160" height="160" alt="Menu11 for Git icon" />

# Menu11 for Git

**Native Git commands in the Windows 11 modern context menu**

[![Windows 11](https://img.shields.io/badge/Windows_11-x64-0078D4?style=flat&labelColor=33201A&logo=windows11&logoColor=white)](https://www.microsoft.com/windows/windows-11)
[![Native Shell](https://img.shields.io/badge/Shell-Native_C%2B%2B-F4512C?style=flat&labelColor=33201A)](src/Menu11.Shell)
[![WinUI 3](https://img.shields.io/badge/Settings-WinUI_3-7A5AF8?style=flat&labelColor=33201A)](src/Menu11.App)
[![.NET 8](https://img.shields.io/badge/.NET-8-512BD4?style=flat&labelColor=33201A&logo=dotnet&logoColor=white)](https://dotnet.microsoft.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-D4A72C?style=flat&labelColor=33201A)](LICENSE)

English | [中文](README_ZH.md)

</div>

Menu11 for Git is a lightweight Windows 11 shell extension that puts Git for
Windows commands directly in Explorer's native modern context menu. It uses the
Git installation already present on the computer and does not bundle, replace,
or reimplement Git.

Explorer shows `Git Bash Here`, `Git GUI Here`, and `Git` as three independent
first-level commands. Enabled operations are direct children of the single
`Git` submenu, so the menu never exceeds two levels. `Menu11.exe` is both the
application and settings center; there is no tray process, service, startup
entry, scheduled task, or resident background component.

> Menu11 for Git is an independent third-party project. It is not affiliated
> with or endorsed by the Git Project or Git for Windows. Git and related marks
> belong to their respective owners.

## Features

### Native Windows 11 integration

- Native out-of-process `IExplorerCommand` shell extension with Sparse Package
  registration—no legacy “Show more options” dependency.
- `Git Bash Here`, `Git GUI Here`, and `Git` all appear at the first level.
- Repository operations: Status, Pull, Fetch, Push, Commit, Log, Branch, and
  Stash.
- File operations: Add, Diff, Log, Blame, and Restore, including safe supported
  multi-selection commands.
- Other operations: Clone and Init.
- One flat `Git` submenu; every operation is a leaf command and cannot create a
  third menu level. The final entry opens `Settings`.

### Settings and language

- Compact WinUI 3 settings center for command visibility, icons, Git location,
  and Explorer refresh.
- English, Simplified Chinese, and System Language modes for both the settings
  center and Explorer commands.
- Automatic Git for Windows detection with an optional custom installation
  location.
- Original branch-and-node visual identity with dedicated small-size command
  icons; it does not copy the official Git logo.

### Updates and process model

- Optional update check when `Menu11.exe` starts; enabled by default and
  available manually from About.
- Updates are read from this repository's published GitHub Releases. Installing
  always requires an explicit user confirmation.
- The updater accepts only three-component `vX.Y.Z` releases and a fixed x64
  Setup filename, then verifies the downloaded installer against its published
  SHA-256 manifest before starting it.
- Shell work is handed to a short-lived native Runner. Menu11 never keeps an
  updater, Runner, tray process, or settings process resident in the background.

## Requirements

### To use Menu11 for Git

- Windows 11 x64, build 22000 or later.
- [Git for Windows](https://gitforwindows.org/) installed before Menu11.

The branded Setup performs a mandatory Git for Windows check before displaying
the install flow and checks again before files are installed. Installation is
blocked when a valid Git for Windows installation cannot be found.

### To build from source

- Visual Studio 2022 Build Tools with the MSVC v143 x64 toolset
- Windows 11 SDK 10.0.26100.0
- .NET SDK 8.0.424
- Node.js 24 and npm when regenerating visual assets
- Inno Setup 6.7 or later when building the installer
- Git for Windows for native integration tests and installer validation

## Downloads and updates

Download the versioned x64 Setup EXE and its `.sha256` file from
[GitHub Releases](https://github.com/CodeH-top/Menu11-For-Git/releases):

```text
Menu11ForGitSetup-X.Y.Z-x64.exe
Menu11ForGitSetup-X.Y.Z-x64.exe.sha256
```

Setup installs Menu11 for the current user under
`%LOCALAPPDATA%\Programs\Menu11 for Git`, registers the three modern context-menu
packages, and creates the selected shortcuts. Setup fixes this location rather
than allowing another drive or network path because Windows applies additional
trust restrictions to Sparse Package external content. Release payloads retain
only English and Simplified Chinese resources. Raw MSIX/Sparse Package files
are implementation details and are not the normal distribution format.

Automatic checks occur only while the settings application is running. Menu11
contacts the public GitHub Releases API, shows the available version, and waits
for the user to choose **Download and install**. The update download can be
cancelled; an incomplete, unexpected, or checksum-mismatched installer is
deleted and never started.

> Current CI artifacts are not Authenticode-signed. Windows may display a
> reputation warning until signed releases are introduced. Verify the published
> SHA-256 file before installation when downloading manually.

## Build

Clone the repository and build the complete x64 solution from PowerShell:

```powershell
git clone git@github.com:CodeH-top/Menu11-For-Git.git
cd Menu11-For-Git
./eng/build.ps1 -Configuration Debug -Platform x64
./eng/test.ps1 -Configuration Debug -Platform x64 -NoBuild
```

Build and validate the branded Release installer:

```powershell
./eng/build.ps1 -Configuration Release -Platform x64 -Rebuild
./eng/test.ps1 -Configuration Release -Platform x64 -NoBuild
./eng/validate-package-manifests.ps1
./eng/build-installer.ps1 -Configuration Release -Platform x64 -NoBuild
```

Regenerate all application, package, installer, and command icons:

```powershell
./eng/build-assets.ps1
```

## Release workflow

The [`Build and publish Release`](.github/workflows/release.yml) workflow runs
when an exact three-component tag such as `v0.1.2` is pushed. It rejects a tag
whose version differs from `eng/Product.props` or whose commit is not contained
in `main`. On the GitHub-hosted Windows 2025 runner it regenerates branding,
rebuilds Release x64, runs native and managed tests, validates all Sparse Package
manifests, builds the Inno Setup installer, generates its SHA-256 manifest, and
publishes both files in one GitHub Release.

The workflow never changes or commits a version. Update these two values first,
commit them to `main`, and only then create the matching tag:

```xml
<Menu11Version>0.1.2</Menu11Version>
<Menu11FileVersion>0.1.2.0</Menu11FileVersion>
```

The fourth component is fixed at zero only for Windows/MSIX technical metadata;
all user-visible versions and release tags use exactly three numbers.

## Architecture

| Component | Responsibility |
| --- | --- |
| `Menu11.exe` | WinUI 3 application, settings center, Git detection, and user-initiated update flow |
| `Menu11.Shell.dll` | Lightweight native COM `IExplorerCommand` implementation loaded out of Explorer's process |
| `Menu11.Runner.exe` | Short-lived native validation and process-launch boundary |
| Sparse Packages | Per-user registration for the three first-level Windows 11 commands |
| Git for Windows | Supplies `git.exe`, Git Bash, Git GUI, and Bash; never bundled by Menu11 |

Selected paths are passed as separate UTF-16 process arguments. They are never
interpolated into a generated shell script. Repository operations use only
fixed Menu11-owned scripts and Bash positional arguments; see
[`docs/runner-protocol.md`](docs/runner-protocol.md).

## Project layout

```text
assets/                         Brand, package, installer, and command icon assets
eng/                            Build, test, stage, package, and installer scripts
installer/                      Bilingual branded Inno Setup project
package/                        Three Sparse Package manifests
src/Menu11.App/                 WinUI 3 settings center and update interaction
src/Menu11.Shell/               Native modern context-menu COM server
src/Menu11.Runner/              Native short-lived command Runner
src/Menu11.Shared.Managed/      Managed settings, Git detection, and updater core
src/Menu11.Shared.Native/       Native settings, detection, launch, and protocol code
tests/                          Native and managed executable test suites
docs/                           Architecture, configuration, branding, and acceptance notes
```

## Privacy and safety

- No telemetry, advertisements, account system, or cloud data storage.
- Settings are per-user values under `HKCU\Software\Menu11ForGit`.
- No service, tray process, scheduled task, startup task, or background updater.
- Update network access occurs only from `Menu11.exe` when automatic checking is
  enabled or the user selects **Check for updates**.
- The shell extension performs small local reads and delegates process creation
  to the Runner to prioritize Explorer stability.
- Uninstall removes Menu11 packages, files, shortcuts, and settings without
  modifying Git for Windows or the user's Git configuration.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for
details.

[Back to top](#menu11-for-git)

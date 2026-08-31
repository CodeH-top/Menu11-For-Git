# MVP acceptance report

Validated on Windows 11 x64 on 2026-08-31.

## Deliverable

- Setup: `Menu11ForGitSetup-0.1.2-x64.exe`
- SHA-256: `FB0C4CA2FE76419ED2CAEE2E4633F93D8B87187AFF6A83E47A44E84EBC6F456F`
- Install scope: current user, under `%LOCALAPPDATA%\Programs\Menu11 for Git`
- Registered packages: `Menu11ForGit.GitBash`, `Menu11ForGit.GitGui`, and
  `Menu11ForGit.GitCommands`

The hash above was independently recalculated from the published GitHub
Release asset and matches its `.sha256` manifest. The `v0.1.2` Release is the
latest non-draft, non-prerelease release and was built by the successful
[`Build and publish Release`](https://github.com/CodeH-top/Menu11-For-Git/actions/runs/33396020958)
workflow.

The per-user install scope keeps the unpackaged executable, external-location
package registration, settings, shortcuts, and uninstaller under the same
Windows user. This avoids registering a per-user Sparse Package from an
elevated administrator context.

## Build and execution

- Debug x64 build and native/managed tests pass.
- Release x64 build and native/managed tests pass.
- Package manifests pass `eng/validate-package-manifests.ps1`.
- The real Git for Windows runner smoke test passes Status, Init, multi-file
  Add, and confirmed Restore with Unicode and shell-significant paths.
- Runner-created Bash processes and Menu11 processes are absent after tests.
- Installed EXE, Runner, Shell DLL, application ICO, and application PNG hashes
  match the Release installer stage.

## Explorer

- `Git`, `Git Bash Here`, and `Git GUI Here` are separate first-level native
  Windows 11 commands.
- Top-level visibility is enforced through package registration because
  Windows 11 does not reliably honor `ECS_HIDDEN` for these commands. Turning
  Git GUI on registered its package, turning it off unregistered it, and a
  forced Explorer refresh removed an intentionally re-registered disabled
  package (`1 -> 0`).
- Repository commands are direct children of `Git` and every child is a leaf;
  no third menu level is exposed.
- The top-level and Git submenu commands use rounded orange square icons with
  white command glyphs. The final submenu item is `Settings` without an
  ellipsis.
- The repository-context screenshot is
  `artifacts/qa/explorer-repository-final-installed-git-submenu.png`.
- The first-level icon screenshot is
  `artifacts/qa/explorer-context-menu-final-installed.png`.
- The final Git-GUI-disabled screenshot is
  `artifacts/qa/git-gui-off-final-0.1.2-context-menu.png`; it retains `Git` and
  `Git Bash Here` and omits `Git GUI Here`.
- Native tests verify unique command GUIDs, dedicated icon resources, COM
  lifetime/unloading, Registry-backed child visibility, and leaf-command
  behavior. Managed tests verify the settings-to-package selection rules.

## Application and branding

- `Menu11.exe` is the application and settings center.
- English, Simplified Chinese, and system-language modes are implemented; the
  installed system-language launch was visually verified in Chinese.
- The application mark uses an original dual-rail commit merge with a subtle
  `11` structure. It is not the official Git diamond or node layout.
- Brand assets cover 16, 20, 24, 32, 48, 64, 128, and 256 pixels plus ICO,
  MSIX, application, README, and installer variants.
- Git, Git Bash, Git GUI, and each Git operation use dedicated small-size
  command icons.
- Settings-center command buttons use a consistent rounded-rectangle style.
  The installed MIT license opens in a scrollable, selectable in-app dialog.
- All non-interactive result bars share a three-second auto-dismiss service,
  covering update checks, Git detection, Explorer refresh, and error results.
  Runtime timing observed the bar at 0.023 seconds, closed it at 3.048 seconds,
  and measured 3.025 seconds of visible time. Confirmation, update-install,
  progress, and license dialogs remain open for explicit user interaction.

## Install, idle, and uninstall

- Setup is bilingual and branded, fixes the install directory instead of
  exposing an unsupported custom-directory page, creates the Start Menu entry,
  registers all three packages, and verifies registration before reporting
  success. The staged payload retains only `en-US` and `zh-CN` culture
  directories.
- Per the later product requirement, Setup blocks when Git for Windows is not
  detected. Detection is enforced in both `InitializeSetup` and
  `PrepareToInstall`.
- No Menu11 Run/RunOnce entry, Startup-folder item, scheduled task, Windows
  service, tray process, `Menu11.exe`, or `Menu11.Runner.exe` remains at idle.
- Silent uninstall removed all packages, the install directory, Start Menu
  folder, uninstall entry, and `HKCU\Software\Menu11ForGit`.
- The Git executable and the user's `.gitconfig` retained identical SHA-256
  hashes across uninstall.

## Environment limitation

Windows Sandbox is not installed on the validation machine, so the compiled
Setup EXE could not be launched on a clean Windows instance with Git absent.
The negative path is covered by the two installer gates and code inspection;
the positive path was exercised by a full install using the detected Git for
Windows installation.

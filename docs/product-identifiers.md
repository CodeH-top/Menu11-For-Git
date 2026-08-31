# Product identifiers

This file is the human-readable reference for identifiers shared by the app,
shell extension, sparse package, and installer. Build-time version values live
in `eng/Product.props` and the native equivalents live in
`src/Menu11.Shared.Native/include/Menu11.Shared/ProductVersion.h`.

| Purpose | Value |
| --- | --- |
| Public product name | Menu11 for Git |
| Internal product name | Menu11ForGit |
| User-visible version | 0.1.2 |
| Target platform | Windows 11 x64 |
| Main application | Menu11.exe |
| Native shell extension | Menu11.Shell.dll |
| Native command runner | Menu11.Runner.exe |
| Package identity name | Menu11ForGit |
| Development package publisher | CN=Menu11ForGit |
| Registry root | HKCU\Software\Menu11ForGit |
| Default install directory | `%LOCALAPPDATA%\Programs\Menu11 for Git` |
| Primary installer | Menu11ForGitSetup-0.1.2-x64.exe |

The package publisher is a development identity. A public release must replace
it with the subject of the production code-signing certificate while keeping
the package identity name stable.

Menu11 uses three-part user-visible versions. Windows PE and MSIX metadata
require four numeric fields, so version 0.1.2 is represented internally as
0.1.2.0 without exposing the trailing zero in the application or installer.

Menu11 for Git is an independent third-party project and is not affiliated with
or endorsed by the Git Project or Git for Windows.

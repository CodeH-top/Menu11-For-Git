# Registry configuration contract

Menu11 stores per-user settings under `HKCU\Software\Menu11ForGit`. The shell
extension performs only small Registry reads and falls back to defaults when a
value is absent or malformed.

| Value | Type | Default | Meaning |
| --- | --- | --- | --- |
| `Enabled` | DWORD | `1` | Enables all Menu11 Explorer commands |
| `GitPath` | REG_SZ | absent | Optional user-selected Git for Windows path |
| `ShowGitBash` | DWORD | `1` | Shows the top-level Git Bash command |
| `ShowGitGUI` | DWORD | `1` | Shows the top-level Git GUI command |
| `EnabledCommands` | DWORD | `0x0000631F` | Enabled Git submenu command bitmask |
| `ShowSettingsCommand` | DWORD | `1` | Shows Settings at the bottom of the Git menu |
| `ShowIcons` | DWORD | `1` | Shows command icons |
| `AutomaticallyCheckForUpdates` | DWORD | `1` | Checks GitHub Releases when `Menu11.exe` starts |

`AutomaticallyCheckForUpdates` is read only by the settings application. The
native Shell extension does not perform network requests and does not read this
value.

## Command bits

| Bit | Command | Default |
| ---: | --- | --- |
| 0 | Status | On |
| 1 | Pull | On |
| 2 | Fetch | On |
| 3 | Push | On |
| 4 | Commit | On |
| 5 | Repository Log | Off |
| 6 | Branch | Off |
| 7 | Stash | Off |
| 8 | File Add | On |
| 9 | File Diff | On |
| 10 | File Log | Off |
| 11 | File Blame | Off |
| 12 | File Restore | Off |
| 13 | Clone | On |
| 14 | Init Repository | On |

Repository, File, and Other are settings-page categories only. Explorer always
places enabled actions directly in the single `Git >` submenu.

# Menu11 visual identity

The Menu11 mark is a compact three-node merge graph. Two upright commit rails
form a subtle `11`, then the right rail makes an angular Git-style merge into
the main line. Removing the fourth endpoint keeps the silhouette readable at
16 px while the dual-rail structure identifies Menu11. The unrotated rounded
Windows tile keeps the composition distinct from the official Git diamond.
The mark is flat, two-color, and has no dark outline.

## Palette

| Role | Color |
| --- | --- |
| Brand tile | `#F4512C` |
| Branch and nodes | `#FFF8F4` |
| Installer accent | `#FFB39F` |
| Installer surface | `#FFFBF9` |
| Installer text | `#4B2922` |

The source assets are `assets/AppIcon.svg` and `assets/InstallerBanner.svg`.
Run `eng/build-assets.ps1` to regenerate every PNG and `AppIcon.ico`. Generated
files are checked in so normal application builds do not require Node.js.

Explorer commands use compact orange rounded-square tiles with white line
symbols in `assets/icons`. Each SVG has a generated multi-resolution ICO
resource and an inset glyph tuned to remain legible at 16 and 20 px. Git Bash,
Git GUI, the Git parent, and every command type use a distinct symbol while the
shared tile makes the command family immediately recognizable.

The mark is original to Menu11 for Git. It must not be replaced with, overlaid
on, or presented as the official Git logo.

# Sparse identity packages

Menu11 installs three tiny sparse identity packages, one for each top-level
Explorer command: Git Bash, Git GUI, and Git. All three reference the same
external installation directory and the same native `Menu11.Shell.dll`.

Windows 11 groups multiple semantic verbs from one package into an
app-attributed flyout. Keeping one semantic Verb per package lets Git Bash and
Git GUI remain direct first-level commands while the Git command uses its own
single, intentional `IExplorerCommand::EnumSubCommands` flyout. The repeated
Verb Id inside each manifest registers that one command for files, folders, and
folder backgrounds; those registrations must continue to share the same Id and
CLSID.

`unvirtualizedResources` ensures the three Shell identities and the unpackaged
settings application share the same `HKCU\Software\Menu11ForGit` settings.

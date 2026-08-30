# Runner protocol

The shell extension starts `Menu11.Runner.exe` with an explicit executable path
and a structured command line:

```text
Menu11.Runner.exe --command <command-id> --path <selected-path> [--path <selected-path> ...]
```

`--path` can be repeated for multi-selection. The Settings command is the only
command that does not require a selected path. The parser accepts at most 512
paths and rejects unknown options, duplicate commands, missing values, and empty
paths.

Stable command IDs are:

```text
bash gui status pull fetch push commit repository-log branch stash
add diff file-log blame restore clone init settings
```

The shell and Runner share the same protocol enum. Every process launch calls
`CreateProcessW` with an explicit application path, UTF-16 arguments, a separate
working directory, and Windows-compatible argument quoting. The Shell extension
never constructs or invokes a command shell.

`Git Bash Here` starts Git for Windows' top-level `git-bash.exe` with its supported
`--cd=<directory>` argument. Repository, file, Clone, and Init commands instead
start Git for Windows' `bin\bash.exe` in a new console as:

```text
bash.exe --login -c <Menu11-owned fixed script> menu11 [selected-path ...]
```

Only fixed scripts compiled into Menu11 are supplied to `-c`. User-controlled
paths are never concatenated or interpolated into a script; they are separate
Windows process arguments and are read only through Bash positional parameters.
Menu11 does not route commands through `cmd.exe` or PowerShell and does not build
scripts from selected file or directory text.

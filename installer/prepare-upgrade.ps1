param(
    [Parameter(Mandatory)]
    [string] $InstallDirectory
)

$ErrorActionPreference = 'Stop'
$shellPath = [System.IO.Path]::GetFullPath((Join-Path $InstallDirectory 'Menu11.Shell.dll'))

# Each sparse identity package hosts the Shell extension in an isolated COM
# surrogate. Stop only surrogates that have loaded our DLL so an upgrade can
# replace the native binary without terminating unrelated COM servers. Keep a
# matching package registration in place: removing and re-registering it makes
# an already-running Explorer cache the extension as absent until Explorer is
# restarted.
Get-Process -Name dllhost -ErrorAction SilentlyContinue | ForEach-Object {
    $surrogate = $_
    try {
        $hostsMenu11 = @($surrogate.Modules) | Where-Object {
            [string]::Equals(
                [System.IO.Path]::GetFullPath($_.FileName),
                $shellPath,
                [System.StringComparison]::OrdinalIgnoreCase)
        }
        if ($hostsMenu11) {
            Stop-Process -Id $surrogate.Id -Force -ErrorAction Stop
        }
    }
    catch {
        # Access to an unrelated elevated surrogate can be denied. It cannot
        # have loaded this current-user installation, so leave it untouched.
    }
}

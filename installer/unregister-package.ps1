param(
    [string] $InstallDirectory
)

$ErrorActionPreference = 'Stop'

if (-not [string]::IsNullOrWhiteSpace($InstallDirectory)) {
    $shellPath = [System.IO.Path]::GetFullPath((Join-Path $InstallDirectory 'Menu11.Shell.dll'))
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
            # Ignore unrelated surrogates that are inaccessible to this user.
        }
    }
}

foreach ($packageName in @(
    'Menu11ForGit',
    'Menu11ForGit.GitBash',
    'Menu11ForGit.GitGui',
    'Menu11ForGit.GitCommands'
)) {
    Get-AppxPackage -Name $packageName -ErrorAction SilentlyContinue |
        Remove-AppxPackage -ErrorAction Stop
}

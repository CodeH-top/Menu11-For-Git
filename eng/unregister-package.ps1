[CmdletBinding()]
param(
    [switch] $RemoveDevelopmentCertificate
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot

foreach ($packageName in @(
    'Menu11ForGit',
    'Menu11ForGit.GitBash',
    'Menu11ForGit.GitGui',
    'Menu11ForGit.GitCommands'
)) {
    Get-AppxPackage -Name $packageName -ErrorAction SilentlyContinue |
        Remove-AppxPackage -ErrorAction Stop
}

if ($RemoveDevelopmentCertificate) {
    $thumbprintPath = Join-Path $repoRoot 'artifacts\dev-signing\thumbprint.txt'
    if (Test-Path -LiteralPath $thumbprintPath) {
        $thumbprint = (Get-Content -LiteralPath $thumbprintPath -Raw).Trim()
        if ($thumbprint -match '^[0-9A-Fa-f]{40}$') {
            Remove-Item -LiteralPath "Cert:\LocalMachine\TrustedPeople\$thumbprint" -Force -ErrorAction SilentlyContinue
            Remove-Item -LiteralPath "Cert:\CurrentUser\TrustedPeople\$thumbprint" -Force -ErrorAction SilentlyContinue
            Remove-Item -LiteralPath "Cert:\CurrentUser\Root\$thumbprint" -Force -ErrorAction SilentlyContinue
        }
    }
}

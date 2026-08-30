[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$toolDirectory = Join-Path $repoRoot 'tools\assets'
$npm = Get-Command npm -ErrorAction Stop

Push-Location $toolDirectory
try {
    if (-not (Test-Path -LiteralPath (Join-Path $toolDirectory 'node_modules\sharp'))) {
        & $npm.Source ci --no-audit --no-fund
        if ($LASTEXITCODE -ne 0) {
            throw "Asset tool dependency restore failed with exit code $LASTEXITCODE."
        }
    }

    & $npm.Source run build
    if ($LASTEXITCODE -ne 0) {
        throw "Asset generation failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Debug',

    [ValidateSet('x64')]
    [string] $Platform = 'x64',

    [switch] $NoBuild
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not $NoBuild) {
    & (Join-Path $PSScriptRoot 'build.ps1') -Configuration $Configuration -Platform $Platform
}

$nativeTests = Join-Path $repoRoot "artifacts\bin\$Configuration\$Platform\Menu11.Native.Tests.exe"
$managedTests = Join-Path $repoRoot "tests\Menu11.Managed.Tests\bin\$Platform\$Configuration\net8.0-windows10.0.26100.0\Menu11.Managed.Tests.exe"

foreach ($testExecutable in @($nativeTests, $managedTests)) {
    if (-not (Test-Path -LiteralPath $testExecutable)) {
        throw "Test executable was not found: $testExecutable"
    }

    & $testExecutable
    if ($LASTEXITCODE -ne 0) {
        throw "Test failed with exit code ${LASTEXITCODE}: $testExecutable"
    }
}

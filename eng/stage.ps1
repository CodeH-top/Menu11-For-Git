[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Release',

    [ValidateSet('x64')]
    [string] $Platform = 'x64',

    [switch] $NoBuild
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not $NoBuild) {
    & (Join-Path $PSScriptRoot 'build.ps1') -Configuration $Configuration -Platform $Platform
}

$stageDirectory = Join-Path $repoRoot "artifacts\stage\$Configuration\$Platform"
$appOutput = Join-Path $repoRoot "src\Menu11.App\bin\$Platform\$Configuration\net8.0-windows10.0.26100.0\win-x64"
$nativeOutput = Join-Path $repoRoot "artifacts\bin\$Configuration\$Platform"

if (-not (Test-Path -LiteralPath $appOutput)) {
    throw "Menu11 application output was not found: $appOutput"
}

if (Test-Path -LiteralPath $stageDirectory) {
    $resolvedStage = (Resolve-Path -LiteralPath $stageDirectory).Path
    $resolvedArtifacts = (Resolve-Path -LiteralPath (Join-Path $repoRoot 'artifacts')).Path
    if (-not $resolvedStage.StartsWith($resolvedArtifacts, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to replace a stage directory outside artifacts: $resolvedStage"
    }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}

$null = New-Item -ItemType Directory -Path $stageDirectory
Copy-Item -Path (Join-Path $appOutput '*') -Destination $stageDirectory -Recurse -Force
Copy-Item -LiteralPath (Join-Path $nativeOutput 'Menu11.Runner.exe') -Destination $stageDirectory -Force
Copy-Item -LiteralPath (Join-Path $nativeOutput 'Menu11.Shell.dll') -Destination $stageDirectory -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'LICENSE') -Destination $stageDirectory -Force

$stageAssets = Join-Path $stageDirectory 'Assets'
$null = New-Item -ItemType Directory -Path $stageAssets -Force
foreach ($asset in @('StoreLogo.png', 'Square44x44Logo.png', 'Square150x150Logo.png')) {
    Copy-Item -LiteralPath (Join-Path $repoRoot "assets\$asset") -Destination $stageAssets -Force
}

Write-Output $stageDirectory

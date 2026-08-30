[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Release',

    [ValidateSet('x64')]
    [string] $Platform = 'x64',

    [string] $CertificatePath,

    [string] $CertificatePassword,

    [switch] $NoBuild
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$installerRoot = Join-Path $repoRoot 'installer'
$stageDirectory = Join-Path $repoRoot "artifacts\stage\$Configuration\$Platform"
$installerStage = Join-Path $repoRoot "artifacts\installer-stage\$Configuration\$Platform"
$packageDirectory = Join-Path $installerStage 'Package'
$installerTools = Join-Path $installerStage 'Installer'
$outputDirectory = Join-Path $installerRoot 'output'
$innoScript = Join-Path $installerRoot 'Menu11ForGit.iss'
$productProps = Join-Path $repoRoot 'eng\Product.props'

if (-not $NoBuild) {
    & (Join-Path $PSScriptRoot 'build.ps1') -Configuration $Configuration -Platform $Platform
}

& (Join-Path $PSScriptRoot 'stage.ps1') -Configuration $Configuration -Platform $Platform -NoBuild | Out-Null
& (Join-Path $PSScriptRoot 'validate-package-manifests.ps1') | Out-Null

if (Test-Path -LiteralPath $installerStage) {
    $resolvedStage = (Resolve-Path -LiteralPath $installerStage).Path
    $resolvedArtifacts = (Resolve-Path -LiteralPath (Join-Path $repoRoot 'artifacts')).Path
    if (-not $resolvedStage.StartsWith($resolvedArtifacts + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to replace installer stage outside artifacts: $resolvedStage"
    }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}

$null = New-Item -ItemType Directory -Path $installerStage -Force
Copy-Item -Path (Join-Path $stageDirectory '*') -Destination $installerStage -Recurse -Force
$null = New-Item -ItemType Directory -Path $packageDirectory -Force
foreach ($packageName in @('GitBash', 'GitGui', 'GitCommands')) {
    Copy-Item `
        -LiteralPath (Join-Path (Join-Path $repoRoot 'package') $packageName) `
        -Destination $packageDirectory `
        -Recurse `
        -Force
}
$null = New-Item -ItemType Directory -Path $installerTools -Force
Copy-Item -LiteralPath (Join-Path $installerRoot 'register-package.ps1') -Destination $installerTools -Force
Copy-Item -LiteralPath (Join-Path $installerRoot 'unregister-package.ps1') -Destination $installerTools -Force

if (-not [string]::IsNullOrWhiteSpace($CertificatePath)) {
    $resolvedCertificate = (Resolve-Path -LiteralPath $CertificatePath).Path
    $sdkVersion = '10.0.26100.0'
    $signTool = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin\$sdkVersion\x64\signtool.exe"
    if (-not (Test-Path -LiteralPath $signTool)) {
        throw "SignTool was not found: $signTool"
    }
    $filesToSign = @(
        (Join-Path $installerStage 'Menu11.exe'),
        (Join-Path $installerStage 'Menu11.Runner.exe'),
        (Join-Path $installerStage 'Menu11.Shell.dll')
    )
    foreach ($file in $filesToSign) {
        & $signTool sign /fd SHA256 /f $resolvedCertificate /p $CertificatePassword $file
        if ($LASTEXITCODE -ne 0) {
            throw "Signing failed for $file with exit code $LASTEXITCODE."
        }
    }
}

$innoCandidates = @(
    (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'),
    (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
    (Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe')
)
$iscc = $innoCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $iscc) {
    throw 'Inno Setup 6.7 or later was not found. Install JRSoftware.InnoSetup with winget.'
}

[xml] $props = Get-Content -LiteralPath $productProps -Raw
$version = [string] $props.Project.PropertyGroup.Menu11Version
if ([string]::IsNullOrWhiteSpace($version)) {
    throw 'Menu11Version was not found in eng\Product.props.'
}

$null = New-Item -ItemType Directory -Path $outputDirectory -Force
& $iscc `
    "/DSourceRoot=$installerStage" `
    "/DOutputRoot=$outputDirectory" `
    "/DProductVersion=$version" `
    $innoScript
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup compilation failed with exit code $LASTEXITCODE."
}

$setupPath = Join-Path $outputDirectory "Menu11ForGitSetup-$version-x64.exe"
if (-not (Test-Path -LiteralPath $setupPath)) {
    throw "Setup output was not found: $setupPath"
}

Write-Output $setupPath

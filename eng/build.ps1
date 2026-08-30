[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Debug',

    [ValidateSet('x64')]
    [string] $Platform = 'x64',

    [switch] $Rebuild
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio Installer (vswhere.exe) was not found.'
}

$visualStudio = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $visualStudio) {
    throw 'Visual Studio 2022 Build Tools with the MSVC x64 toolset was not found.'
}

$msbuild = Join-Path $visualStudio 'MSBuild\Current\Bin\MSBuild.exe'
$dotnet = Get-Command dotnet -ErrorAction Stop
$sdkVersion = (& $dotnet.Source --version).Trim()
$sdkRoot = Join-Path (Split-Path -Parent $dotnet.Source) "sdk\$sdkVersion\Sdks"

if (-not (Test-Path -LiteralPath $sdkRoot)) {
    throw "The .NET SDK targets directory was not found: $sdkRoot"
}

$previousSdkPath = $env:MSBuildSDKsPath
$previousWorkloadResolver = $env:MSBuildEnableWorkloadResolver
try {
    $env:MSBuildSDKsPath = $sdkRoot
    $env:MSBuildEnableWorkloadResolver = 'false'
    $target = if ($Rebuild) { '/t:Rebuild' } else { '/t:Build' }
    & $msbuild (Join-Path $repoRoot 'Menu11ForGit.sln') /restore $target /m /nr:false "/p:Configuration=$Configuration" "/p:Platform=$Platform" /verbosity:minimal
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE."
    }
}
finally {
    $env:MSBuildSDKsPath = $previousSdkPath
    $env:MSBuildEnableWorkloadResolver = $previousWorkloadResolver
}

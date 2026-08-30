[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Release',

    [string] $PackageDirectory,

    [switch] $Development
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$stageDirectory = Join-Path $repoRoot "artifacts\stage\$Configuration\x64"
$resolvedStage = (Resolve-Path -LiteralPath $stageDirectory).Path
$packageDefinitions = @(
    @{ Name = 'Menu11ForGit.GitBash'; Manifest = 'GitBash\AppxManifest.xml'; FileName = 'Menu11ForGit.GitBash.identity.msix' },
    @{ Name = 'Menu11ForGit.GitGui'; Manifest = 'GitGui\AppxManifest.xml'; FileName = 'Menu11ForGit.GitGui.identity.msix' },
    @{ Name = 'Menu11ForGit.GitCommands'; Manifest = 'GitCommands\AppxManifest.xml'; FileName = 'Menu11ForGit.GitCommands.identity.msix' }
)
$allPackageNames = @('Menu11ForGit') + @($packageDefinitions | ForEach-Object Name)

function Send-ShellAssociationChanged {
    try {
        $nativeMethods = Add-Type `
            -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("shell32.dll", ExactSpelling = true)]
public static extern void SHChangeNotify(
    int eventId,
    uint flags,
    System.IntPtr item1,
    System.IntPtr item2);
'@ `
            -Name 'ShellNativeMethods' `
            -Namespace 'Menu11ForGitInstaller' `
            -PassThru

        $nativeMethods::SHChangeNotify(
            0x08000000,
            0x1000,
            [System.IntPtr]::Zero,
            [System.IntPtr]::Zero)
    }
    catch {
        Write-Warning "Could not notify Explorer about the updated context-menu registration: $_"
    }
}

foreach ($packageName in $allPackageNames) {
    Get-AppxPackage -Name $packageName -ErrorAction SilentlyContinue |
        Remove-AppxPackage -ErrorAction Stop
}

try {
    if ($Development) {
        & (Join-Path $PSScriptRoot 'validate-package-manifests.ps1') | Out-Null
        foreach ($definition in $packageDefinitions) {
            $manifestPath = (Resolve-Path -LiteralPath (
                Join-Path (Join-Path $repoRoot 'package') $definition.Manifest)).Path
            Add-AppxPackage `
                -Path $manifestPath `
                -Register `
                -ExternalLocation $resolvedStage `
                -ForceApplicationShutdown `
                -ErrorAction Stop
        }
    }
    else {
        if ([string]::IsNullOrWhiteSpace($PackageDirectory)) {
            $PackageDirectory = Join-Path $repoRoot "artifacts\package\$Configuration\x64"
        }
        $resolvedPackageDirectory = (Resolve-Path -LiteralPath $PackageDirectory).Path
        foreach ($definition in $packageDefinitions) {
            $resolvedPackage = (Resolve-Path -LiteralPath (
                Join-Path $resolvedPackageDirectory $definition.FileName)).Path
            Add-AppxPackage `
                -Path $resolvedPackage `
                -ExternalLocation $resolvedStage `
                -ForceApplicationShutdown `
                -ErrorAction Stop
        }
    }
}
catch {
    foreach ($definition in $packageDefinitions) {
        Get-AppxPackage -Name $definition.Name -ErrorAction SilentlyContinue |
            Remove-AppxPackage -ErrorAction SilentlyContinue
    }
    throw
}

$registeredPackages = foreach ($definition in $packageDefinitions) {
    $package = Get-AppxPackage -Name $definition.Name -ErrorAction Stop
    if ($package.Status -ne 'Ok') {
        throw "$($definition.Name) registration finished with status $($package.Status)."
    }
    $package
}
Send-ShellAssociationChanged
$registeredPackages

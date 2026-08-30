param(
    [Parameter(Mandatory)]
    [string] $InstallDirectory
)

$ErrorActionPreference = 'Stop'
$packageRoot = Join-Path $InstallDirectory 'Package'
$packageDefinitions = @(
    @{ Name = 'Menu11ForGit.GitBash'; Manifest = 'GitBash\AppxManifest.xml' },
    @{ Name = 'Menu11ForGit.GitGui'; Manifest = 'GitGui\AppxManifest.xml' },
    @{ Name = 'Menu11ForGit.GitCommands'; Manifest = 'GitCommands\AppxManifest.xml' }
)

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

        # Microsoft documents SHCNE_ASSOCCHANGED as the notification that
        # invalidates the Shell icon and thumbnail cache after handlers change.
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

function Get-NormalizedPath {
    param([Parameter(Mandatory)][string] $Path)

    return [System.IO.Path]::GetFullPath($Path).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar)
}

function Test-ReusableRegistration {
    param(
        [Parameter(Mandatory)] $Package,
        [Parameter(Mandatory)][string] $ManifestPath
    )

    [xml] $manifest = Get-Content -LiteralPath $ManifestPath -Raw
    $expectedVersion = [version] $manifest.Package.Identity.Version
    $expectedLocation = Get-NormalizedPath (Split-Path -Parent $ManifestPath)
    $registeredLocation = Get-NormalizedPath $Package.InstallLocation

    return (
        $Package.Status -eq 'Ok' -and
        ([version] $Package.Version) -eq $expectedVersion -and
        [string]::Equals(
            $registeredLocation,
            $expectedLocation,
            [System.StringComparison]::OrdinalIgnoreCase))
}

foreach ($definition in $packageDefinitions) {
    $manifestPath = Join-Path $packageRoot $definition.Manifest
    if (-not (Test-Path -LiteralPath $manifestPath)) {
        throw "Menu11 package manifest was not found: $manifestPath"
    }
}

# Remove an obsolete single-package identity from early development builds.
Get-AppxPackage -Name 'Menu11ForGit' -ErrorAction SilentlyContinue |
    Remove-AppxPackage -ErrorAction Stop

$packagesToRegister = [System.Collections.Generic.List[object]]::new()
foreach ($definition in $packageDefinitions) {
    $manifestPath = Join-Path $packageRoot $definition.Manifest
    $packages = @(Get-AppxPackage -Name $definition.Name -ErrorAction SilentlyContinue)
    if ($packages.Count -eq 1 -and
        (Test-ReusableRegistration -Package $packages[0] -ManifestPath $manifestPath)) {
        continue
    }

    $packages | Remove-AppxPackage -ErrorAction Stop
    $packagesToRegister.Add($definition)
}

$registeredThisRun = [System.Collections.Generic.List[string]]::new()
try {
    foreach ($definition in $packagesToRegister) {
        Add-AppxPackage `
            -Path (Join-Path $packageRoot $definition.Manifest) `
            -Register `
            -ExternalLocation $InstallDirectory `
            -ForceApplicationShutdown `
            -ErrorAction Stop
        $registeredThisRun.Add($definition.Name)
    }
}
catch {
    foreach ($packageName in $registeredThisRun) {
        Get-AppxPackage -Name $packageName -ErrorAction SilentlyContinue |
            Remove-AppxPackage -ErrorAction SilentlyContinue
    }
    throw
}

foreach ($definition in $packageDefinitions) {
    $package = Get-AppxPackage -Name $definition.Name -ErrorAction Stop
    if ($package.Status -ne 'Ok') {
        throw "$($definition.Name) registration finished with status $($package.Status)."
    }
}

Send-ShellAssociationChanged

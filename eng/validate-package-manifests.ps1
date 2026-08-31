[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
[xml] $productProps = Get-Content -LiteralPath (Join-Path $repoRoot 'eng\Product.props') -Raw
$expectedPackageVersion = [string] $productProps.Project.PropertyGroup.Menu11FileVersion
$expectedLanguages = @('en-us', 'zh-cn')
$definitions = @(
    @{ Identity = 'Menu11ForGit.GitBash'; RelativePath = 'GitBash\AppxManifest.xml' },
    @{ Identity = 'Menu11ForGit.GitGui'; RelativePath = 'GitGui\AppxManifest.xml' },
    @{ Identity = 'Menu11ForGit.GitCommands'; RelativePath = 'GitCommands\AppxManifest.xml' }
)
$requiredContexts = @('*', 'Directory', 'Directory\Background')
$allClassIds = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase)

foreach ($definition in $definitions) {
    $manifestPath = Join-Path (Join-Path $repoRoot 'package') $definition.RelativePath
    if (-not (Test-Path -LiteralPath $manifestPath)) {
        throw "Sparse package manifest was not found: $manifestPath"
    }

    [xml] $manifest = Get-Content -LiteralPath $manifestPath -Raw
    $namespaces = [System.Xml.XmlNamespaceManager]::new($manifest.NameTable)
    $namespaces.AddNamespace('f', 'http://schemas.microsoft.com/appx/manifest/foundation/windows10')
    $namespaces.AddNamespace('com', 'http://schemas.microsoft.com/appx/manifest/com/windows10')
    $namespaces.AddNamespace('desktop5', 'http://schemas.microsoft.com/appx/manifest/desktop/windows10/5')
    $namespaces.AddNamespace('rescap', 'http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities')

    $identity = $manifest.SelectSingleNode('/f:Package/f:Identity', $namespaces)
    if ($null -eq $identity -or $identity.Name -ne $definition.Identity) {
        throw "Unexpected package identity in ${manifestPath}: $($identity.Name)"
    }
    if ([string] $identity.Version -cne $expectedPackageVersion) {
        throw "$($definition.Identity) version $($identity.Version) does not match $expectedPackageVersion."
    }

    $languages = @(
        $manifest.SelectNodes('/f:Package/f:Resources/f:Resource', $namespaces) |
            ForEach-Object { ([string] $_.Language).ToLowerInvariant() } |
            Sort-Object -Unique
    )
    if (Compare-Object -ReferenceObject $expectedLanguages -DifferenceObject $languages) {
        throw "$($definition.Identity) must declare only en-us and zh-cn resources."
    }

    $applications = @($manifest.SelectNodes('/f:Package/f:Applications/f:Application', $namespaces))
    $verbs = @($manifest.SelectNodes('//desktop5:Verb', $namespaces))
    $classes = @($manifest.SelectNodes('//com:Class', $namespaces))
    if ($applications.Count -ne 1 -or $classes.Count -ne 1 -or $verbs.Count -ne 3) {
        throw "$($definition.Identity) must contain one application, one COM class, and three context registrations."
    }

    $verbIds = @($verbs | ForEach-Object Id | Select-Object -Unique)
    $verbClassIds = @($verbs | ForEach-Object Clsid | Select-Object -Unique)
    $contexts = @($manifest.SelectNodes('//desktop5:ItemType', $namespaces) | ForEach-Object Type | Sort-Object)
    if ($verbIds.Count -ne 1 -or $verbClassIds.Count -ne 1) {
        throw "$($definition.Identity) must reuse one Verb Id and one CLSID across all item types."
    }
    if (Compare-Object -ReferenceObject ($requiredContexts | Sort-Object) -DifferenceObject $contexts) {
        throw "$($definition.Identity) does not register exactly the required file, folder, and background contexts."
    }
    if ($classes[0].Id -ne $verbClassIds[0]) {
        throw "$($definition.Identity) Verb CLSID does not match its COM class."
    }
    if (-not $allClassIds.Add([string] $classes[0].Id)) {
        throw "Shell COM CLSID is duplicated across sparse packages: $($classes[0].Id)"
    }

    $capabilities = @($manifest.SelectNodes('/f:Package/f:Capabilities/rescap:Capability', $namespaces) | ForEach-Object Name)
    foreach ($requiredCapability in @('runFullTrust', 'unvirtualizedResources')) {
        if ($requiredCapability -notin $capabilities) {
            throw "$($definition.Identity) is missing capability $requiredCapability."
        }
    }
}

Write-Output "Validated three single-Verb Menu11 sparse package manifests at version $expectedPackageVersion."

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Release',

    [string] $CertificatePath,

    [string] $CertificatePassword
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$sdkVersion = '10.0.26100.0'
$sdkBin = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin\$sdkVersion\x64"
$makeAppx = Join-Path $sdkBin 'makeappx.exe'
$signTool = Join-Path $sdkBin 'signtool.exe'
$outputDirectory = Join-Path $repoRoot "artifacts\package\$Configuration\x64"
$packageDefinitions = @(
    @{ Source = 'GitBash'; FileName = 'Menu11ForGit.GitBash.identity.msix' },
    @{ Source = 'GitGui'; FileName = 'Menu11ForGit.GitGui.identity.msix' },
    @{ Source = 'GitCommands'; FileName = 'Menu11ForGit.GitCommands.identity.msix' }
)

foreach ($tool in @($makeAppx, $signTool)) {
    if (-not (Test-Path -LiteralPath $tool)) {
        throw "Required Windows SDK tool was not found: $tool"
    }
}

$null = & (Join-Path $PSScriptRoot 'stage.ps1') -Configuration $Configuration -Platform x64
& (Join-Path $PSScriptRoot 'validate-package-manifests.ps1') | Out-Null
$null = New-Item -ItemType Directory -Path $outputDirectory -Force

$resolvedCertificate = if ([string]::IsNullOrWhiteSpace($CertificatePath)) {
    $null
}
else {
    (Resolve-Path -LiteralPath $CertificatePath).Path
}
$packagePaths = foreach ($definition in $packageDefinitions) {
    $sourceDirectory = Join-Path (Join-Path $repoRoot 'package') $definition.Source
    $packagePath = Join-Path $outputDirectory $definition.FileName
    $makeAppxOutput = & $makeAppx pack /o /d $sourceDirectory /nv /p $packagePath 2>&1
    $makeAppxExitCode = $LASTEXITCODE
    foreach ($line in $makeAppxOutput) {
        Write-Host $line
    }
    if ($makeAppxExitCode -ne 0) {
        throw "MakeAppx failed for $($definition.Source) with exit code $makeAppxExitCode."
    }

    if ($null -ne $resolvedCertificate) {
        $signToolOutput = & $signTool sign /fd SHA256 /f $resolvedCertificate /p $CertificatePassword $packagePath 2>&1
        $signToolExitCode = $LASTEXITCODE
        foreach ($line in $signToolOutput) {
            Write-Host $line
        }
        if ($signToolExitCode -ne 0) {
            throw "SignTool failed for $packagePath with exit code $signToolExitCode."
        }
    }

    $packagePath
}

Write-Output $packagePaths

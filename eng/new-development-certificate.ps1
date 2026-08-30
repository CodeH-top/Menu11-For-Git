[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $Password,

    [switch] $Trust
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$outputDirectory = Join-Path $repoRoot 'artifacts\dev-signing'
$pfxPath = Join-Path $outputDirectory 'Menu11ForGit.Development.pfx'
$cerPath = Join-Path $outputDirectory 'Menu11ForGit.Development.cer'
$thumbprintPath = Join-Path $outputDirectory 'thumbprint.txt'
$securePassword = ConvertTo-SecureString -String $Password -AsPlainText -Force

$null = New-Item -ItemType Directory -Path $outputDirectory -Force
$certificate = New-SelfSignedCertificate `
    -Type Custom `
    -Subject 'CN=Menu11ForGit' `
    -FriendlyName 'Menu11 for Git local development signing' `
    -KeyAlgorithm RSA `
    -KeyLength 2048 `
    -HashAlgorithm SHA256 `
    -KeyUsage DigitalSignature `
    -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3') `
    -NotAfter (Get-Date).AddYears(2) `
    -CertStoreLocation 'Cert:\CurrentUser\My'

try {
    $null = Export-PfxCertificate -Cert $certificate -FilePath $pfxPath -Password $securePassword -Force
    $null = Export-Certificate -Cert $certificate -FilePath $cerPath -Force
    if ($Trust) {
        $principal = [Security.Principal.WindowsPrincipal]::new(
            [Security.Principal.WindowsIdentity]::GetCurrent())
        if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
            throw 'Trusting an MSIX development certificate requires an elevated PowerShell session.'
        }
        $null = Import-Certificate `
            -FilePath $cerPath `
            -CertStoreLocation 'Cert:\LocalMachine\TrustedPeople'
    }
    Set-Content -LiteralPath $thumbprintPath -Value $certificate.Thumbprint -NoNewline
}
finally {
    Remove-Item -LiteralPath "Cert:\CurrentUser\My\$($certificate.Thumbprint)" -Force -ErrorAction SilentlyContinue
}

[pscustomobject]@{
    PfxPath = $pfxPath
    CerPath = $cerPath
    Thumbprint = $certificate.Thumbprint
}

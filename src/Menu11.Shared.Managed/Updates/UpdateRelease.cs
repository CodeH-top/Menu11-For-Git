namespace Menu11.Shared.Updates;

public sealed record UpdateRelease(
    ProductVersion Version,
    string TagName,
    Uri ReleasePageUri,
    string InstallerFileName,
    Uri InstallerUri,
    long InstallerSize,
    Uri ChecksumUri);

public sealed record UpdateCheckResult(
    ProductVersion CurrentVersion,
    UpdateRelease? AvailableRelease)
{
    public bool IsUpdateAvailable => AvailableRelease is not null;
}

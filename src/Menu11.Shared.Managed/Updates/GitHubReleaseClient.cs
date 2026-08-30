using System.Net;
using System.Net.Http.Json;
using System.Net.Http.Headers;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Menu11.Shared.Updates;

public sealed class GitHubReleaseClient
{
    public const string RepositoryOwner = "CodeH-top";
    public const string RepositoryName = "Menu11-For-Git";

    private static readonly Uri LatestReleaseUri = new(
        $"https://api.github.com/repos/{RepositoryOwner}/{RepositoryName}/releases/latest");
    private const int MaximumMetadataBytes = 1024 * 1024;
    private const long MaximumInstallerBytes = 512L * 1024 * 1024;
    private readonly HttpClient _httpClient;

    public GitHubReleaseClient(HttpClient httpClient)
    {
        _httpClient = httpClient ?? throw new ArgumentNullException(nameof(httpClient));
    }

    public async Task<UpdateCheckResult> CheckForUpdatesAsync(
        ProductVersion currentVersion,
        CancellationToken cancellationToken = default)
    {
        var release = await GetLatestReleaseAsync(cancellationToken).ConfigureAwait(false);
        return new UpdateCheckResult(
            currentVersion,
            release is not null && release.Version > currentVersion ? release : null);
    }

    public async Task<UpdateRelease?> GetLatestReleaseAsync(
        CancellationToken cancellationToken = default)
    {
        using var request = new HttpRequestMessage(HttpMethod.Get, LatestReleaseUri);
        request.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
        request.Headers.UserAgent.ParseAdd("Menu11-for-Git-Updater");
        request.Headers.Add("X-GitHub-Api-Version", "2026-03-10");

        using var response = await _httpClient.SendAsync(
            request,
            HttpCompletionOption.ResponseHeadersRead,
            cancellationToken).ConfigureAwait(false);
        if (response.StatusCode == HttpStatusCode.NotFound)
        {
            return null;
        }

        response.EnsureSuccessStatusCode();
        if (response.Content.Headers.ContentLength is > MaximumMetadataBytes)
        {
            throw new UpdateException("The GitHub release response is unexpectedly large.");
        }

        await response.Content.LoadIntoBufferAsync(MaximumMetadataBytes)
            .ConfigureAwait(false);

        GitHubReleaseResponse? payload;
        try
        {
            payload = await response.Content.ReadFromJsonAsync<GitHubReleaseResponse>(
                cancellationToken: cancellationToken).ConfigureAwait(false);
        }
        catch (JsonException exception)
        {
            throw new UpdateException("The GitHub release response is invalid.", exception);
        }

        if (payload is null || payload.Draft || payload.Prerelease ||
            string.IsNullOrWhiteSpace(payload.TagName) ||
            !ProductVersion.TryParse(payload.TagName, out var version))
        {
            throw new UpdateException("The latest GitHub release does not use a supported stable version tag.");
        }

        var tagName = payload.TagName;
        var normalizedTag = $"v{version}";
        if (!string.Equals(tagName, normalizedTag, StringComparison.Ordinal))
        {
            throw new UpdateException("The latest GitHub release tag is not normalized.");
        }

        var installerFileName = $"Menu11ForGitSetup-{version}-x64.exe";
        var checksumFileName = $"{installerFileName}.sha256";
        var installerAsset = FindAsset(payload.Assets, installerFileName);
        var checksumAsset = FindAsset(payload.Assets, checksumFileName);

        if (installerAsset.Size is <= 0 or > MaximumInstallerBytes)
        {
            throw new UpdateException("The release installer size is invalid.");
        }

        var releasePageUri = ParseTrustedReleaseUri(payload.HtmlUrl, tagName, assetName: null);
        var installerUri = ParseTrustedReleaseUri(
            installerAsset.DownloadUrl,
            tagName,
            installerFileName);
        var checksumUri = ParseTrustedReleaseUri(
            checksumAsset.DownloadUrl,
            tagName,
            checksumFileName);

        return new UpdateRelease(
            version,
            tagName,
            releasePageUri,
            installerFileName,
            installerUri,
            installerAsset.Size,
            checksumUri);
    }

    private static GitHubAssetResponse FindAsset(
        IReadOnlyList<GitHubAssetResponse>? assets,
        string expectedName)
    {
        var matches = assets?
            .Where(asset => string.Equals(asset.Name, expectedName, StringComparison.Ordinal))
            .ToArray() ?? [];
        if (matches.Length != 1)
        {
            throw new UpdateException($"The release must contain exactly one {expectedName} asset.");
        }

        return matches[0];
    }

    private static Uri ParseTrustedReleaseUri(string? value, string tagName, string? assetName)
    {
        if (!Uri.TryCreate(value, UriKind.Absolute, out var uri) ||
            uri.Scheme != Uri.UriSchemeHttps ||
            !uri.Host.Equals("github.com", StringComparison.OrdinalIgnoreCase) ||
            !string.IsNullOrEmpty(uri.UserInfo) ||
            !uri.IsDefaultPort)
        {
            throw new UpdateException("The release contains an untrusted download address.");
        }

        var expectedPath = assetName is null
            ? $"/{RepositoryOwner}/{RepositoryName}/releases/tag/{tagName}"
            : $"/{RepositoryOwner}/{RepositoryName}/releases/download/{tagName}/{assetName}";
        if (!Uri.UnescapeDataString(uri.AbsolutePath).Equals(
            expectedPath,
            StringComparison.OrdinalIgnoreCase))
        {
            throw new UpdateException("The release contains an unexpected download address.");
        }

        return uri;
    }

    private sealed class GitHubReleaseResponse
    {
        [JsonPropertyName("tag_name")]
        public string? TagName { get; init; }

        [JsonPropertyName("html_url")]
        public string? HtmlUrl { get; init; }

        [JsonPropertyName("draft")]
        public bool Draft { get; init; }

        [JsonPropertyName("prerelease")]
        public bool Prerelease { get; init; }

        [JsonPropertyName("assets")]
        public List<GitHubAssetResponse>? Assets { get; init; }
    }

    private sealed class GitHubAssetResponse
    {
        [JsonPropertyName("name")]
        public string? Name { get; init; }

        [JsonPropertyName("browser_download_url")]
        public string? DownloadUrl { get; init; }

        [JsonPropertyName("size")]
        public long Size { get; init; }
    }
}

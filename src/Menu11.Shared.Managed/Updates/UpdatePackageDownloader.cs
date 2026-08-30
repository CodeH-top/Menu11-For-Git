using System.Buffers;
using System.Net.Http.Headers;
using System.Security.Cryptography;

namespace Menu11.Shared.Updates;

public sealed class UpdatePackageDownloader
{
    private const long MaximumInstallerBytes = 512L * 1024 * 1024;
    private const int MaximumChecksumBytes = 16 * 1024;
    private readonly HttpClient _httpClient;
    private readonly string _downloadRoot;

    public UpdatePackageDownloader(HttpClient httpClient, string? downloadRoot = null)
    {
        _httpClient = httpClient ?? throw new ArgumentNullException(nameof(httpClient));
        _downloadRoot = downloadRoot ?? Path.Combine(
            Path.GetTempPath(),
            "Menu11ForGit",
            "Updates");
    }

    public async Task<string> DownloadAndVerifyAsync(
        UpdateRelease release,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(release);
        if (release.InstallerSize is <= 0 or > MaximumInstallerBytes)
        {
            throw new UpdateException("The release installer size is invalid.");
        }

        var checksumText = await DownloadChecksumAsync(release.ChecksumUri, cancellationToken)
            .ConfigureAwait(false);
        if (!UpdateChecksum.TryParse(checksumText, release.InstallerFileName, out var expectedHash))
        {
            throw new UpdateException("The release checksum file is invalid.");
        }

        var versionDirectory = Path.GetFullPath(Path.Combine(_downloadRoot, release.Version.ToString()));
        var root = Path.GetFullPath(_downloadRoot)
            .TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar) + Path.DirectorySeparatorChar;
        if (!versionDirectory.StartsWith(root, StringComparison.OrdinalIgnoreCase))
        {
            throw new UpdateException("The update download location is invalid.");
        }

        Directory.CreateDirectory(versionDirectory);
        var finalPath = Path.Combine(versionDirectory, release.InstallerFileName);
        var partialPath = finalPath + ".download";

        try
        {
            var actualHash = await DownloadInstallerAsync(
                release,
                partialPath,
                cancellationToken).ConfigureAwait(false);
            if (!CryptographicOperations.FixedTimeEquals(actualHash, expectedHash))
            {
                throw new UpdateException("The downloaded installer failed SHA-256 verification.");
            }

            File.Move(partialPath, finalPath, overwrite: true);
            return finalPath;
        }
        catch
        {
            TryDelete(partialPath);
            throw;
        }
    }

    private async Task<string> DownloadChecksumAsync(
        Uri checksumUri,
        CancellationToken cancellationToken)
    {
        using var response = await SendAsync(checksumUri, cancellationToken).ConfigureAwait(false);
        if (response.Content.Headers.ContentLength is > MaximumChecksumBytes)
        {
            throw new UpdateException("The release checksum file is unexpectedly large.");
        }

        await response.Content.LoadIntoBufferAsync(MaximumChecksumBytes)
            .ConfigureAwait(false);
        return await response.Content.ReadAsStringAsync(cancellationToken).ConfigureAwait(false);
    }

    private async Task<byte[]> DownloadInstallerAsync(
        UpdateRelease release,
        string partialPath,
        CancellationToken cancellationToken)
    {
        using var response = await SendAsync(release.InstallerUri, cancellationToken)
            .ConfigureAwait(false);
        var contentLength = response.Content.Headers.ContentLength;
        if (contentLength.HasValue && contentLength.Value != release.InstallerSize)
        {
            throw new UpdateException("The installer size does not match the GitHub release metadata.");
        }

        await using var input = await response.Content.ReadAsStreamAsync(cancellationToken)
            .ConfigureAwait(false);
        await using var output = new FileStream(
            partialPath,
            FileMode.Create,
            FileAccess.Write,
            FileShare.None,
            bufferSize: 64 * 1024,
            useAsync: true);
        using var hasher = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);

        var buffer = ArrayPool<byte>.Shared.Rent(64 * 1024);
        long totalBytes = 0;
        try
        {
            while (true)
            {
                var bytesRead = await input.ReadAsync(buffer, cancellationToken).ConfigureAwait(false);
                if (bytesRead == 0)
                {
                    break;
                }

                totalBytes += bytesRead;
                if (totalBytes > release.InstallerSize || totalBytes > MaximumInstallerBytes)
                {
                    throw new UpdateException("The downloaded installer is larger than expected.");
                }

                hasher.AppendData(buffer, 0, bytesRead);
                await output.WriteAsync(buffer.AsMemory(0, bytesRead), cancellationToken)
                    .ConfigureAwait(false);
            }
        }
        finally
        {
            ArrayPool<byte>.Shared.Return(buffer);
        }

        if (totalBytes != release.InstallerSize)
        {
            throw new UpdateException("The downloaded installer is incomplete.");
        }

        await output.FlushAsync(cancellationToken).ConfigureAwait(false);
        return hasher.GetHashAndReset();
    }

    private async Task<HttpResponseMessage> SendAsync(
        Uri uri,
        CancellationToken cancellationToken)
    {
        using var request = new HttpRequestMessage(HttpMethod.Get, uri);
        request.Headers.UserAgent.ParseAdd("Menu11-for-Git-Updater");
        request.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/octet-stream"));
        var response = await _httpClient.SendAsync(
            request,
            HttpCompletionOption.ResponseHeadersRead,
            cancellationToken).ConfigureAwait(false);
        try
        {
            response.EnsureSuccessStatusCode();
            var finalUri = response.RequestMessage?.RequestUri;
            if (finalUri is null || !IsTrustedGitHubDownloadUri(finalUri))
            {
                throw new UpdateException("GitHub redirected the update to an untrusted address.");
            }

            return response;
        }
        catch
        {
            response.Dispose();
            throw;
        }
    }

    private static bool IsTrustedGitHubDownloadUri(Uri uri)
    {
        if (uri.Scheme != Uri.UriSchemeHttps ||
            !string.IsNullOrEmpty(uri.UserInfo) ||
            !uri.IsDefaultPort)
        {
            return false;
        }

        return uri.Host.Equals("github.com", StringComparison.OrdinalIgnoreCase) ||
            uri.Host.Equals("release-assets.githubusercontent.com", StringComparison.OrdinalIgnoreCase) ||
            uri.Host.EndsWith(".githubusercontent.com", StringComparison.OrdinalIgnoreCase);
    }

    private static void TryDelete(string path)
    {
        try
        {
            File.Delete(path);
        }
        catch (IOException)
        {
        }
        catch (UnauthorizedAccessException)
        {
        }
    }
}

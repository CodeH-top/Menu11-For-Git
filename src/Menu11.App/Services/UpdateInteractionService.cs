using System.Diagnostics;
using System.Net;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Menu11.Shared.Updates;

namespace Menu11.App.Services;

public enum UpdateInteractionStatus
{
    UpToDate,
    UpdateDeclined,
    DownloadCancelled,
    InstallerStarted,
    Failed,
    Busy,
}

public sealed record UpdateInteractionResult(
    UpdateInteractionStatus Status,
    ProductVersion? LatestVersion = null);

public sealed class UpdateInteractionService
{
    private static readonly HttpClient SharedHttpClient = CreateHttpClient();
    private readonly GitHubReleaseClient _releaseClient;
    private readonly UpdatePackageDownloader _downloader;
    private readonly SemaphoreSlim _operationGate = new(1, 1);

    public UpdateInteractionService()
        : this(
            new GitHubReleaseClient(SharedHttpClient),
            new UpdatePackageDownloader(SharedHttpClient))
    {
    }

    internal UpdateInteractionService(
        GitHubReleaseClient releaseClient,
        UpdatePackageDownloader downloader)
    {
        _releaseClient = releaseClient;
        _downloader = downloader;
    }

    public async Task<UpdateInteractionResult> CheckAndPromptAsync(
        XamlRoot xamlRoot,
        string currentVersionText,
        bool automatic)
    {
        ArgumentNullException.ThrowIfNull(xamlRoot);
        if (!await _operationGate.WaitAsync(0))
        {
            return new UpdateInteractionResult(UpdateInteractionStatus.Busy);
        }

        try
        {
            if (!ProductVersion.TryParse(currentVersionText, out var currentVersion))
            {
                return new UpdateInteractionResult(UpdateInteractionStatus.Failed);
            }

            var check = await _releaseClient.CheckForUpdatesAsync(currentVersion);
            if (!check.IsUpdateAvailable)
            {
                return new UpdateInteractionResult(UpdateInteractionStatus.UpToDate);
            }

            var release = check.AvailableRelease!;
            var confirmation = new ContentDialog
            {
                XamlRoot = xamlRoot,
                Title = LocalizationService.GetString("UpdateAvailableTitle"),
                Content = new TextBlock
                {
                    Text = LocalizationService.Format(
                        "UpdateAvailableMessage",
                        currentVersion,
                        release.Version),
                    TextWrapping = TextWrapping.Wrap,
                    MaxWidth = 440,
                },
                PrimaryButtonText = LocalizationService.GetString("DownloadAndInstall"),
                CloseButtonText = LocalizationService.GetString("Cancel"),
                DefaultButton = ContentDialogButton.Primary,
            };
            if (await confirmation.ShowAsync() != ContentDialogResult.Primary)
            {
                return new UpdateInteractionResult(
                    UpdateInteractionStatus.UpdateDeclined,
                    release.Version);
            }

            return await DownloadAndLaunchAsync(xamlRoot, release);
        }
        catch (OperationCanceledException)
        {
            return new UpdateInteractionResult(UpdateInteractionStatus.DownloadCancelled);
        }
        catch (Exception exception) when (
            exception is HttpRequestException or IOException or UnauthorizedAccessException or
            UpdateException or InvalidOperationException)
        {
            return new UpdateInteractionResult(UpdateInteractionStatus.Failed);
        }
        finally
        {
            _operationGate.Release();
        }
    }

    private async Task<UpdateInteractionResult> DownloadAndLaunchAsync(
        XamlRoot xamlRoot,
        UpdateRelease release)
    {
        using var cancellation = new CancellationTokenSource();
        var progressRing = new ProgressRing
        {
            IsActive = true,
            IsIndeterminate = true,
            Width = 24,
            Height = 24,
            VerticalAlignment = VerticalAlignment.Center,
        };
        var message = new TextBlock
        {
            Text = LocalizationService.GetString("DownloadingUpdateMessage"),
            TextWrapping = TextWrapping.Wrap,
            VerticalAlignment = VerticalAlignment.Center,
        };
        var content = new StackPanel
        {
            Orientation = Orientation.Horizontal,
            Spacing = 12,
        };
        content.Children.Add(progressRing);
        content.Children.Add(message);

        var progressDialog = new ContentDialog
        {
            XamlRoot = xamlRoot,
            Title = LocalizationService.GetString("DownloadingUpdateTitle"),
            Content = content,
            CloseButtonText = LocalizationService.GetString("Cancel"),
        };
        progressDialog.CloseButtonClick += (_, _) => cancellation.Cancel();
        var dialogOperation = progressDialog.ShowAsync();

        string installerPath;
        try
        {
            await Task.Yield();
            installerPath = await _downloader.DownloadAndVerifyAsync(
                release,
                cancellation.Token);
        }
        finally
        {
            try
            {
                progressDialog.Hide();
            }
            catch (InvalidOperationException)
            {
            }

            try
            {
                await dialogOperation;
            }
            catch (InvalidOperationException)
            {
            }
        }

        var process = Process.Start(new ProcessStartInfo
        {
            FileName = installerPath,
            WorkingDirectory = Path.GetDirectoryName(installerPath)!,
            UseShellExecute = true,
        });
        if (process is null)
        {
            throw new InvalidOperationException("Windows did not start the verified update installer.");
        }

        Application.Current.Exit();
        return new UpdateInteractionResult(
            UpdateInteractionStatus.InstallerStarted,
            release.Version);
    }

    private static HttpClient CreateHttpClient()
    {
        var handler = new HttpClientHandler
        {
            AllowAutoRedirect = true,
            AutomaticDecompression = DecompressionMethods.All,
            MaxAutomaticRedirections = 5,
        };
        return new HttpClient(handler)
        {
            Timeout = TimeSpan.FromMinutes(10),
        };
    }
}

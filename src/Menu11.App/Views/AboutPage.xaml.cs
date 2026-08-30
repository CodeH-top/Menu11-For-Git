using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Menu11.App.ViewModels;
using Menu11.App.Services;
using Windows.System;

namespace Menu11.App.Views;

public sealed partial class AboutPage : Page
{
    public SettingsViewModel ViewModel { get; }

    public AboutPage()
    {
        ViewModel = ((App)Application.Current).Settings;
        InitializeComponent();
    }

    private async void GitHub_Click(object sender, RoutedEventArgs e)
    {
        await LaunchUriAsync(ViewModel.RepositoryUrl, LocalizationService.GetString("UnableOpenProjectPage"));
    }

    private async void CheckForUpdates_Click(object sender, RoutedEventArgs e)
    {
        UpdatesButton.IsEnabled = false;
        ShowStatus(
            LocalizationService.GetString("CheckingForUpdatesTitle"),
            LocalizationService.GetString("CheckingForUpdatesMessage"),
            InfoBarSeverity.Informational);

        try
        {
            var app = (App)Application.Current;
            var result = await app.Updates.CheckAndPromptAsync(
                XamlRoot,
                ViewModel.ProductVersion,
                automatic: false);
            switch (result.Status)
            {
                case UpdateInteractionStatus.UpToDate:
                    ShowStatus(
                        LocalizationService.GetString("UpToDateTitle"),
                        LocalizationService.Format("UpToDateMessage", ViewModel.ProductVersion),
                        InfoBarSeverity.Success);
                    break;
                case UpdateInteractionStatus.UpdateDeclined:
                    ShowStatus(
                        LocalizationService.GetString("UpdateAvailableTitle"),
                        LocalizationService.Format(
                            "UpdateAvailableLaterMessage",
                            result.LatestVersion ?? default),
                        InfoBarSeverity.Informational);
                    break;
                case UpdateInteractionStatus.DownloadCancelled:
                    ShowStatus(
                        LocalizationService.GetString("UpdateCancelledTitle"),
                        LocalizationService.GetString("UpdateCancelledMessage"),
                        InfoBarSeverity.Informational);
                    break;
                case UpdateInteractionStatus.Failed:
                    ShowStatus(
                        LocalizationService.GetString("UpdateFailedTitle"),
                        LocalizationService.GetString("UpdateFailedMessage"),
                        InfoBarSeverity.Error);
                    break;
                case UpdateInteractionStatus.Busy:
                    ShowStatus(
                        LocalizationService.GetString("UpdateBusyTitle"),
                        LocalizationService.GetString("UpdateBusyMessage"),
                        InfoBarSeverity.Informational);
                    break;
            }
        }
        finally
        {
            UpdatesButton.IsEnabled = ViewModel.HasRepositoryUrl;
        }
    }

    private async void License_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var licensePath = Path.Combine(AppContext.BaseDirectory, "LICENSE");
            var licenseText = await File.ReadAllTextAsync(licensePath);
            var licenseBlock = new TextBlock
            {
                Text = licenseText,
                FontFamily = new Microsoft.UI.Xaml.Media.FontFamily("Consolas"),
                FontSize = 12,
                IsTextSelectionEnabled = true,
                TextWrapping = TextWrapping.Wrap,
            };
            var dialog = new ContentDialog
            {
                XamlRoot = XamlRoot,
                Title = LocalizationService.GetString("LicenseDialogTitle"),
                Content = new ScrollViewer
                {
                    Content = licenseBlock,
                    MaxHeight = 430,
                    Width = 540,
                    VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
                },
                CloseButtonText = LocalizationService.GetString("Close"),
                DefaultButton = ContentDialogButton.Close,
            };
            await dialog.ShowAsync();
            return;
        }
        catch (Exception exception) when (
            exception is IOException or UnauthorizedAccessException or InvalidOperationException)
        {
        }

        ShowError(LocalizationService.GetString("UnableOpenLicense"));
    }

    private async Task LaunchUriAsync(string? value, string errorMessage)
    {
        if (Uri.TryCreate(value, UriKind.Absolute, out var uri) &&
            await Launcher.LaunchUriAsync(uri))
        {
            return;
        }

        ShowError(errorMessage);
    }

    private void ShowError(string message)
    {
        TransientInfoBarService.Show(
            ActionResultBar,
            LocalizationService.GetString("ActionFailed"),
            message,
            InfoBarSeverity.Error);
    }

    private void ShowStatus(string title, string message, InfoBarSeverity severity)
    {
        TransientInfoBarService.Show(ActionResultBar, title, message, severity);
    }
}

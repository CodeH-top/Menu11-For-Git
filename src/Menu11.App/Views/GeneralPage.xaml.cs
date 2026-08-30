using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Menu11.App.Services;
using Menu11.App.ViewModels;
using Menu11.Shared.Settings;
using System.Diagnostics;

namespace Menu11.App.Views;

public sealed partial class GeneralPage : Page
{
    public SettingsViewModel ViewModel { get; }

    public GeneralPage()
    {
        ViewModel = ((App)Application.Current).Settings;
        InitializeComponent();
        SelectCurrentLanguage();
    }

    private void RefreshExplorer_Click(object sender, RoutedEventArgs e)
    {
        ExplorerIntegrationService.Refresh();
        ShowResult(
            LocalizationService.GetString("ExplorerRefreshedTitle"),
            LocalizationService.GetString("ExplorerRefreshedMessage"),
            InfoBarSeverity.Success);
    }

    private async void RestoreDefaults_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new ContentDialog
        {
            Title = LocalizationService.GetString("RestoreDefaultsDialogTitle"),
            Content = LocalizationService.GetString("RestoreDefaultsDialogContent"),
            PrimaryButtonText = LocalizationService.GetString("RestoreDefaultsDialogPrimary"),
            CloseButtonText = LocalizationService.GetString("Cancel"),
            DefaultButton = ContentDialogButton.Close,
            XamlRoot = XamlRoot,
        };

        if (await dialog.ShowAsync() != ContentDialogResult.Primary)
        {
            return;
        }

        ViewModel.RestoreDefaults();
        SelectCurrentLanguage();
        ExplorerIntegrationService.Refresh();
        ShowResult(
            LocalizationService.GetString("DefaultsRestoredTitle"),
            string.Empty,
            InfoBarSeverity.Success);
    }

    private void LanguageBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (LanguageBox.SelectedItem is not ComboBoxItem item || item.Tag is not string language ||
            Menu11Language.Normalize(language) == ViewModel.Language)
        {
            return;
        }

        ViewModel.Language = language;
        LocalizationService.ApplyLanguage(language);
        ExplorerIntegrationService.Refresh();
        try
        {
            var executable = Environment.ProcessPath ??
                Path.Combine(AppContext.BaseDirectory, "Menu11.exe");
            Process.Start(new ProcessStartInfo
            {
                FileName = executable,
                WorkingDirectory = AppContext.BaseDirectory,
                UseShellExecute = false,
            });
            ((App)Application.Current).MainWindow?.Close();
        }
        catch (Exception exception)
        {
            ShowResult(
                LocalizationService.GetString("ActionFailed"),
                exception.Message,
                InfoBarSeverity.Error);
        }
    }

    private void ShowResult(string title, string message, InfoBarSeverity severity)
    {
        TransientInfoBarService.Show(ResultBar, title, message, severity);
    }

    private void SelectCurrentLanguage()
    {
        foreach (var option in LanguageBox.Items.OfType<ComboBoxItem>())
        {
            if (option.Tag is string language && language == ViewModel.Language)
            {
                LanguageBox.SelectedItem = option;
                return;
            }
        }
    }
}

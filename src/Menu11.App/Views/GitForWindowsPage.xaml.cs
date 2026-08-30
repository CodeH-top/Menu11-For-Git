using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Menu11.App.Services;
using Menu11.App.ViewModels;
using Windows.Storage.Pickers;

namespace Menu11.App.Views;

public sealed partial class GitForWindowsPage : Page
{
    public SettingsViewModel ViewModel { get; }

    public GitForWindowsPage()
    {
        ViewModel = ((App)Application.Current).Settings;
        InitializeComponent();
    }

    private async void Browse_Click(object sender, RoutedEventArgs e)
    {
        var picker = new FolderPicker();
        picker.FileTypeFilter.Add("*");

        var window = ((App)Application.Current).MainWindow;
        if (window is null)
        {
            return;
        }

        WinRT.Interop.InitializeWithWindow.Initialize(
            picker,
            WinRT.Interop.WindowNative.GetWindowHandle(window));
        var folder = await picker.PickSingleFolderAsync();
        if (folder is null)
        {
            return;
        }

        GitPathBox.Text = folder.Path;
        ApplyPath(folder.Path);
    }

    private void UsePath_Click(object sender, RoutedEventArgs e)
    {
        ApplyPath(GitPathBox.Text);
    }

    private void Automatic_Click(object sender, RoutedEventArgs e)
    {
        ViewModel.UseAutomaticGitDetection();
        GitPathBox.Text = string.Empty;
        ShowPathResult(LocalizationService.GetString("AutomaticDetectionEnabled"), InfoBarSeverity.Success);
    }

    private void DetectAgain_Click(object sender, RoutedEventArgs e)
    {
        ViewModel.RefreshGitDetection();
        ShowPathResult(ViewModel.GitStatus, InfoBarSeverity.Informational);
    }

    private void ApplyPath(string path)
    {
        if (ViewModel.TrySetGitPath(path))
        {
            GitPathBox.Text = ViewModel.ConfiguredGitPath;
            ShowPathResult(LocalizationService.GetString("GitLocationSaved"), InfoBarSeverity.Success);
            return;
        }

        ShowPathResult(LocalizationService.GetString("GitLocationNotFound"), InfoBarSeverity.Error);
    }

    private void ShowPathResult(string title, InfoBarSeverity severity)
    {
        TransientInfoBarService.Show(PathResultBar, title, string.Empty, severity);
    }
}

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Menu11.App.ViewModels;

namespace Menu11.App.Views;

public sealed partial class GitMenuPage : Page
{
    public SettingsViewModel ViewModel { get; }

    public GitMenuPage()
    {
        ViewModel = ((App)Application.Current).Settings;
        InitializeComponent();
    }

    private void SelectAll_Click(object sender, RoutedEventArgs e)
    {
        ViewModel.SetAllGitCommands(enabled: true);
    }

    private void ClearAll_Click(object sender, RoutedEventArgs e)
    {
        ViewModel.SetAllGitCommands(enabled: false);
    }
}

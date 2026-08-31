using Microsoft.UI.Xaml;
using Menu11.App.Services;
using Menu11.App.ViewModels;
using Menu11.Shared.Settings;

namespace Menu11.App;

public partial class App : Application
{
    public MainWindow? MainWindow { get; private set; }
    public SettingsViewModel Settings { get; }
    public UpdateInteractionService Updates { get; } = new();
    public ContextMenuRegistrationService ContextMenuRegistration { get; } = new();

    public App()
    {
        var settingsStore = new RegistrySettingsStore();
        var settings = settingsStore.Load();
        LocalizationService.ApplyLanguage(settings.Language);
        InitializeComponent();
        Settings = new SettingsViewModel(settings, settingsStore);
        Settings.ShellSettingsChanged += ContextMenuRegistration.RequestSynchronization;
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        MainWindow = new MainWindow();
        MainWindow.Activate();
        ContextMenuRegistration.RequestSynchronization(Settings.CurrentSettings);
        if (Settings.AutomaticallyCheckForUpdates)
        {
            _ = CheckForUpdatesAfterLaunchAsync();
        }
    }

    private async Task CheckForUpdatesAfterLaunchAsync()
    {
        await Task.Delay(800);
        if (MainWindow?.Content is FrameworkElement root && root.XamlRoot is not null)
        {
            await Updates.CheckAndPromptAsync(
                root.XamlRoot,
                Settings.ProductVersion,
                automatic: true);
        }
    }
}

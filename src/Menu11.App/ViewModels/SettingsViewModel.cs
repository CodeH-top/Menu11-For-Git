using System.ComponentModel;
using System.Reflection;
using System.Runtime.CompilerServices;
using Menu11.Shared.Git;
using Menu11.Shared.Settings;
using Menu11.App.Services;

namespace Menu11.App.ViewModels;

public sealed class SettingsViewModel : INotifyPropertyChanged
{
    private readonly ISettingsStore _settingsStore;
    private readonly GitDetectionService _gitDetection = new();
    private Menu11Settings _settings;
    private GitInstallation? _gitInstallation;
    private readonly IReadOnlyList<GitCommandSettingViewModel> _allCommandSettings;

    public SettingsViewModel()
        : this(new RegistrySettingsStore())
    {
    }

    public SettingsViewModel(Menu11Settings settings)
        : this(settings, new RegistrySettingsStore())
    {
    }

    public SettingsViewModel(Menu11Settings settings, ISettingsStore settingsStore)
    {
        ArgumentNullException.ThrowIfNull(settings);
        ArgumentNullException.ThrowIfNull(settingsStore);
        _settingsStore = settingsStore;
        _settings = settings;
        RepositoryCommands = new[]
        {
            Command(GitCommands.Status, "CommandStatus"),
            Command(GitCommands.Pull, "CommandPull"),
            Command(GitCommands.Fetch, "CommandFetch"),
            Command(GitCommands.Push, "CommandPush"),
            Command(GitCommands.Commit, "CommandCommit"),
            Command(GitCommands.RepositoryLog, "CommandLog"),
            Command(GitCommands.Branch, "CommandBranch"),
            Command(GitCommands.Stash, "CommandStash"),
        };
        FileCommands = new[]
        {
            Command(GitCommands.FileAdd, "CommandAdd"),
            Command(GitCommands.FileDiff, "CommandDiff"),
            Command(GitCommands.FileLog, "CommandLog"),
            Command(GitCommands.FileBlame, "CommandBlame"),
            Command(GitCommands.FileRestore, "CommandRestore"),
        };
        OtherCommands = new[]
        {
            Command(GitCommands.Clone, "CommandClone"),
            Command(GitCommands.Init, "CommandInit"),
        };
        _allCommandSettings = RepositoryCommands
            .Concat(FileCommands)
            .Concat(OtherCommands)
            .ToArray();
        RefreshGitDetection();
    }

    private SettingsViewModel(ISettingsStore settingsStore)
        : this(settingsStore.Load(), settingsStore)
    {
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    public event Action<Menu11Settings>? ShellSettingsChanged;

    public bool Enabled
    {
        get => _settings.Enabled;
        set => Update(_settings with { Enabled = value }, nameof(Enabled), affectsShell: true);
    }

    public bool ShowGitBash
    {
        get => _settings.ShowGitBash;
        set => Update(_settings with { ShowGitBash = value }, nameof(ShowGitBash), affectsShell: true);
    }

    public bool ShowGitGui
    {
        get => _settings.ShowGitGui;
        set => Update(_settings with { ShowGitGui = value }, nameof(ShowGitGui), affectsShell: true);
    }

    public bool ShowSettingsCommand
    {
        get => _settings.ShowSettingsCommand;
        set => Update(
            _settings with { ShowSettingsCommand = value },
            nameof(ShowSettingsCommand),
            affectsShell: true);
    }

    public bool ShowIcons
    {
        get => _settings.ShowIcons;
        set => Update(_settings with { ShowIcons = value }, nameof(ShowIcons), affectsShell: true);
    }

    public bool AutomaticallyCheckForUpdates
    {
        get => _settings.AutomaticallyCheckForUpdates;
        set => Update(
            _settings with { AutomaticallyCheckForUpdates = value },
            nameof(AutomaticallyCheckForUpdates));
    }

    public string Language
    {
        get => _settings.Language;
        set => Update(
            _settings with { Language = Menu11Language.Normalize(value) },
            nameof(Language));
    }

    public string ConfiguredGitPath => _settings.GitPath ?? string.Empty;
    public Menu11Settings CurrentSettings => _settings;
    public string GitStatus => LocalizationService.GetString(
        _gitInstallation is null ? "GitStatusNotDetected" : "GitStatusDetected");
    public string GitInstallRoot => _gitInstallation?.InstallRoot ?? LocalizationService.GetString("NotDetected");
    public string GitVersion => string.IsNullOrWhiteSpace(_gitInstallation?.Version)
        ? LocalizationService.GetString("NotAvailable")
        : _gitInstallation.Version;
    public string GitDetectionSourceLabel => _gitInstallation is null
        ? LocalizationService.GetString("NotAvailable")
        : SourceName(_gitInstallation.Source);
    public string ProductVersion =>
        Assembly.GetExecutingAssembly().GetName().Version?.ToString(3) ?? "0.0.0";
    public string ProductVersionLabel => LocalizationService.Format("VersionFormat", ProductVersion);
    public string? RepositoryUrl => ReadAssemblyMetadata("RepositoryUrl");
    public string? ReleasesUrl => RepositoryUrl is null
        ? null
        : $"{RepositoryUrl.TrimEnd('/')}/releases";
    public bool HasRepositoryUrl => RepositoryUrl is not null;

    public IReadOnlyList<GitCommandSettingViewModel> RepositoryCommands { get; }
    public IReadOnlyList<GitCommandSettingViewModel> FileCommands { get; }
    public IReadOnlyList<GitCommandSettingViewModel> OtherCommands { get; }

    public int RepositoryEnabledCount => CountEnabled(
        GitCommands.Status | GitCommands.Pull | GitCommands.Fetch | GitCommands.Push |
        GitCommands.Commit | GitCommands.RepositoryLog | GitCommands.Branch | GitCommands.Stash);
    public int FileEnabledCount => CountEnabled(
        GitCommands.FileAdd | GitCommands.FileDiff | GitCommands.FileLog |
        GitCommands.FileBlame | GitCommands.FileRestore);
    public int OtherEnabledCount => CountEnabled(GitCommands.Clone | GitCommands.Init);

    public string RepositorySummary => LocalizationService.Format("EnabledSummaryFormat", RepositoryEnabledCount, 8);
    public string FileSummary => LocalizationService.Format("EnabledSummaryFormat", FileEnabledCount, 5);
    public string OtherSummary => LocalizationService.Format("EnabledSummaryFormat", OtherEnabledCount, 2);

    public void RefreshGitDetection()
    {
        _gitInstallation = _gitDetection.Detect(_settings.GitPath);
        NotifyGitProperties();
    }

    public bool TrySetGitPath(string path)
    {
        var installation = _gitDetection.InspectCandidate(path, GitDetectionSource.UserSelected);
        if (installation is null)
        {
            return false;
        }

        _settings = _settings with { GitPath = installation.InstallRoot };
        _settingsStore.Save(_settings);
        _gitInstallation = installation;
        OnPropertyChanged(nameof(ConfiguredGitPath));
        NotifyGitProperties();
        return true;
    }

    public void UseAutomaticGitDetection()
    {
        _settings = _settings with { GitPath = null };
        _settingsStore.Save(_settings);
        OnPropertyChanged(nameof(ConfiguredGitPath));
        RefreshGitDetection();
    }

    public void RestoreDefaults()
    {
        _settingsStore.Reset();
        _settings = _settingsStore.Load();
        RefreshGitDetection();
        RefreshCommandSettings();
        NotifyAllSettingsProperties();
        ShellSettingsChanged?.Invoke(_settings);
    }

    public void SetAllGitCommands(bool enabled)
    {
        var commands = enabled ? GitCommands.All : GitCommands.None;
        if (_settings.EnabledCommands == commands)
        {
            return;
        }

        _settings = _settings with { EnabledCommands = commands };
        _settingsStore.Save(_settings);
        RefreshCommandSettings();
        NotifyCommandSummaries();
        ShellSettingsChanged?.Invoke(_settings);
    }

    internal bool IsCommandEnabled(GitCommands command)
    {
        return _settings.IsCommandEnabled(command);
    }

    internal void SetCommandEnabled(GitCommands command, bool enabled)
    {
        var commands = enabled
            ? _settings.EnabledCommands | command
            : _settings.EnabledCommands & ~command;
        commands &= GitCommands.All;
        if (commands == _settings.EnabledCommands)
        {
            return;
        }

        _settings = _settings with { EnabledCommands = commands };
        _settingsStore.Save(_settings);
        NotifyCommandSummaries();
        ShellSettingsChanged?.Invoke(_settings);
    }

    private int CountEnabled(GitCommands commands)
    {
        return System.Numerics.BitOperations.PopCount((uint)(_settings.EnabledCommands & commands));
    }

    private void RefreshCommandSettings()
    {
        foreach (var command in _allCommandSettings)
        {
            command.Refresh();
        }
    }

    private void NotifyCommandSummaries()
    {
        OnPropertyChanged(nameof(RepositorySummary));
        OnPropertyChanged(nameof(FileSummary));
        OnPropertyChanged(nameof(OtherSummary));
    }

    private void Update(Menu11Settings updated, string propertyName, bool affectsShell = false)
    {
        if (_settings == updated)
        {
            return;
        }

        _settings = updated;
        _settingsStore.Save(_settings);
        OnPropertyChanged(propertyName);
        if (affectsShell)
        {
            ShellSettingsChanged?.Invoke(_settings);
        }
    }

    private void NotifyAllSettingsProperties()
    {
        foreach (var property in new[]
        {
            nameof(Enabled),
            nameof(ShowGitBash),
            nameof(ShowGitGui),
            nameof(ShowSettingsCommand),
            nameof(ShowIcons),
            nameof(AutomaticallyCheckForUpdates),
            nameof(Language),
            nameof(ConfiguredGitPath),
            nameof(RepositorySummary),
            nameof(FileSummary),
            nameof(OtherSummary),
        })
        {
            OnPropertyChanged(property);
        }
    }

    private void NotifyGitProperties()
    {
        OnPropertyChanged(nameof(GitStatus));
        OnPropertyChanged(nameof(GitInstallRoot));
        OnPropertyChanged(nameof(GitVersion));
        OnPropertyChanged(nameof(GitDetectionSourceLabel));
    }

    private GitCommandSettingViewModel Command(GitCommands command, string resourceKey) =>
        new(this, command, LocalizationService.GetString(resourceKey));

    private static string SourceName(GitDetectionSource source) => LocalizationService.GetString(source switch
    {
        GitDetectionSource.MachineRegistry64 => "SourceMachineRegistry64",
        GitDetectionSource.MachineRegistry32 => "SourceMachineRegistry32",
        GitDetectionSource.UserRegistry64 => "SourceUserRegistry64",
        GitDetectionSource.UserRegistry32 => "SourceUserRegistry32",
        GitDetectionSource.PathEnvironment => "SourcePath",
        GitDetectionSource.CommonLocation => "SourceCommonLocation",
        GitDetectionSource.UserSelected => "SourceUserSelected",
        _ => "SourceUnknown",
    });

    private static string? ReadAssemblyMetadata(string key)
    {
        var value = Assembly.GetExecutingAssembly()
            .GetCustomAttributes<AssemblyMetadataAttribute>()
            .FirstOrDefault(attribute => attribute.Key == key)
            ?.Value;
        return string.IsNullOrWhiteSpace(value) ? null : value;
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}

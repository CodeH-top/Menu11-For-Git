namespace Menu11.Shared.Settings;

public sealed record Menu11Settings
{
    public bool Enabled { get; init; } = true;
    public bool ShowGitBash { get; init; } = true;
    public bool ShowGitGui { get; init; } = true;
    public GitCommands EnabledCommands { get; init; } = GitCommandDefaults.Enabled;
    public bool ShowSettingsCommand { get; init; } = true;
    public bool ShowIcons { get; init; } = true;
    public bool AutomaticallyCheckForUpdates { get; init; } = true;
    public string? GitPath { get; init; }
    public string Language { get; init; } = Menu11Language.System;

    public bool IsCommandEnabled(GitCommands command) => (EnabledCommands & command) != 0;
}

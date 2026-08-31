namespace Menu11.Shared.Settings;

public readonly record struct ContextMenuPackageSelection(
    bool GitBash,
    bool GitGui,
    bool GitCommands)
{
    public static ContextMenuPackageSelection FromSettings(Menu11Settings settings)
    {
        ArgumentNullException.ThrowIfNull(settings);

        return new ContextMenuPackageSelection(
            GitBash: settings.Enabled && settings.ShowGitBash,
            GitGui: settings.Enabled && settings.ShowGitGui,
            GitCommands: settings.Enabled &&
                (settings.EnabledCommands != global::Menu11.Shared.Settings.GitCommands.None ||
                    settings.ShowSettingsCommand));
    }
}

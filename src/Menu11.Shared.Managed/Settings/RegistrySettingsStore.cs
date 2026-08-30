using System.Security;
using Microsoft.Win32;

namespace Menu11.Shared.Settings;

public sealed class RegistrySettingsStore : ISettingsStore
{
    public const string DefaultSubkey = @"Software\Menu11ForGit";

    private readonly string _subkey;

    public RegistrySettingsStore(string subkey = DefaultSubkey)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(subkey);
        _subkey = subkey;
    }

    public Menu11Settings Load()
    {
        try
        {
            using var key = Registry.CurrentUser.OpenSubKey(_subkey, writable: false);
            if (key is null)
            {
                return new Menu11Settings();
            }

            return new Menu11Settings
            {
                Enabled = ReadBoolean(key, "Enabled", defaultValue: true),
                ShowGitBash = ReadBoolean(key, "ShowGitBash", defaultValue: true),
                ShowGitGui = ReadBoolean(key, "ShowGitGUI", defaultValue: true),
                EnabledCommands = ReadCommands(key),
                ShowSettingsCommand = ReadBoolean(key, "ShowSettingsCommand", defaultValue: true),
                ShowIcons = ReadBoolean(key, "ShowIcons", defaultValue: true),
                AutomaticallyCheckForUpdates = ReadBoolean(
                    key,
                    "AutomaticallyCheckForUpdates",
                    defaultValue: true),
                GitPath = ReadString(key, "GitPath"),
                Language = Menu11Language.Normalize(ReadString(key, "Language")),
            };
        }
        catch (Exception exception) when (
            exception is IOException or SecurityException or UnauthorizedAccessException)
        {
            return new Menu11Settings();
        }
    }

    public void Save(Menu11Settings settings)
    {
        ArgumentNullException.ThrowIfNull(settings);
        using var key = Registry.CurrentUser.CreateSubKey(_subkey, writable: true) ??
            throw new InvalidOperationException($"Unable to create HKCU\\{_subkey}.");

        key.SetValue("Enabled", settings.Enabled ? 1 : 0, RegistryValueKind.DWord);
        key.SetValue("ShowGitBash", settings.ShowGitBash ? 1 : 0, RegistryValueKind.DWord);
        key.SetValue("ShowGitGUI", settings.ShowGitGui ? 1 : 0, RegistryValueKind.DWord);
        key.SetValue("EnabledCommands", (int)(settings.EnabledCommands & GitCommands.All), RegistryValueKind.DWord);
        key.SetValue("ShowSettingsCommand", settings.ShowSettingsCommand ? 1 : 0, RegistryValueKind.DWord);
        key.SetValue("ShowIcons", settings.ShowIcons ? 1 : 0, RegistryValueKind.DWord);
        key.SetValue(
            "AutomaticallyCheckForUpdates",
            settings.AutomaticallyCheckForUpdates ? 1 : 0,
            RegistryValueKind.DWord);
        key.SetValue("Language", Menu11Language.Normalize(settings.Language), RegistryValueKind.String);

        if (string.IsNullOrWhiteSpace(settings.GitPath))
        {
            key.DeleteValue("GitPath", throwOnMissingValue: false);
        }
        else
        {
            key.SetValue("GitPath", settings.GitPath, RegistryValueKind.String);
        }
    }

    public void Reset()
    {
        Registry.CurrentUser.DeleteSubKeyTree(_subkey, throwOnMissingSubKey: false);
    }

    private static bool ReadBoolean(RegistryKey key, string name, bool defaultValue)
    {
        return key.GetValue(name) switch
        {
            0 => false,
            1 => true,
            _ => defaultValue,
        };
    }

    private static GitCommands ReadCommands(RegistryKey key)
    {
        if (key.GetValue("EnabledCommands") is not int value)
        {
            return GitCommandDefaults.Enabled;
        }

        return (GitCommands)((uint)value & (uint)GitCommands.All);
    }

    private static string? ReadString(RegistryKey key, string name)
    {
        var value = key.GetValue(name) as string;
        return string.IsNullOrWhiteSpace(value) ? null : value;
    }
}

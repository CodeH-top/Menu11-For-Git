using System.ComponentModel;
using System.Runtime.CompilerServices;
using Menu11.Shared.Settings;

namespace Menu11.App.ViewModels;

public sealed class GitCommandSettingViewModel : INotifyPropertyChanged
{
    private readonly SettingsViewModel _owner;

    internal GitCommandSettingViewModel(
        SettingsViewModel owner,
        GitCommands command,
        string displayName)
    {
        _owner = owner;
        Command = command;
        DisplayName = displayName;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public GitCommands Command { get; }
    public string DisplayName { get; }

    public bool IsEnabled
    {
        get => _owner.IsCommandEnabled(Command);
        set
        {
            if (value == IsEnabled)
            {
                return;
            }

            _owner.SetCommandEnabled(Command, value);
            OnPropertyChanged();
        }
    }

    internal void Refresh()
    {
        OnPropertyChanged(nameof(IsEnabled));
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}

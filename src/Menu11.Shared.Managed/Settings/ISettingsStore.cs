namespace Menu11.Shared.Settings;

public interface ISettingsStore
{
    Menu11Settings Load();

    void Save(Menu11Settings settings);

    void Reset();
}

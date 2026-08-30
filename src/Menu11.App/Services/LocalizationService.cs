using System.Globalization;
using Menu11.Shared.Settings;
using Microsoft.Windows.ApplicationModel.Resources;

namespace Menu11.App.Services;

public static class LocalizationService
{
    private static readonly object SyncRoot = new();
    private static ResourceManager? _manager;
    private static ResourceContext? _context;
    private static ResourceMap? _resources;

    public static string CurrentLanguage { get; private set; } = Menu11Language.English;

    public static void ApplyLanguage(string preference)
    {
        var language = ResolveLanguage(preference);
        CurrentLanguage = language;

        Microsoft.Windows.Globalization.ApplicationLanguages.PrimaryLanguageOverride = language;
        var culture = CultureInfo.GetCultureInfo(language);
        CultureInfo.DefaultThreadCurrentCulture = culture;
        CultureInfo.DefaultThreadCurrentUICulture = culture;
        CultureInfo.CurrentCulture = culture;
        CultureInfo.CurrentUICulture = culture;

        lock (SyncRoot)
        {
            _manager = new ResourceManager();
            _context = _manager.CreateResourceContext();
            _context.QualifierValues[KnownResourceQualifierName.Language] = language;
            _resources = _manager.MainResourceMap.GetSubtree("Resources");
        }
    }

    public static string ResolveLanguage(string preference)
    {
        var normalized = Menu11Language.Normalize(preference);
        if (normalized != Menu11Language.System)
        {
            return normalized;
        }

        return CultureInfo.InstalledUICulture.TwoLetterISOLanguageName.Equals(
            "zh",
            StringComparison.OrdinalIgnoreCase)
            ? Menu11Language.SimplifiedChinese
            : Menu11Language.English;
    }

    public static string GetString(string key)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(key);
        lock (SyncRoot)
        {
            if (_resources is null || _context is null)
            {
                ApplyLanguage(Menu11Language.System);
            }

            return _resources!.GetValue(key, _context!).ValueAsString;
        }
    }

    public static string Format(string key, params object[] values) =>
        string.Format(CultureInfo.CurrentCulture, GetString(key), values);
}

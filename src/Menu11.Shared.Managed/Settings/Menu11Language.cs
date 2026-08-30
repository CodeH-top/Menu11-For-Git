namespace Menu11.Shared.Settings;

public static class Menu11Language
{
    public const string System = "system";
    public const string English = "en-US";
    public const string SimplifiedChinese = "zh-CN";

    public static string Normalize(string? value) => value switch
    {
        English => English,
        SimplifiedChinese => SimplifiedChinese,
        _ => System,
    };
}

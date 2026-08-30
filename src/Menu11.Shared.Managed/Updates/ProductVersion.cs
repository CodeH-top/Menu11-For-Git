using System.Globalization;

namespace Menu11.Shared.Updates;

public readonly record struct ProductVersion(int Major, int Minor, int Patch) : IComparable<ProductVersion>
{
    public int CompareTo(ProductVersion other)
    {
        var major = Major.CompareTo(other.Major);
        if (major != 0)
        {
            return major;
        }

        var minor = Minor.CompareTo(other.Minor);
        return minor != 0 ? minor : Patch.CompareTo(other.Patch);
    }

    public static bool TryParse(string? value, out ProductVersion version)
    {
        version = default;
        if (string.IsNullOrWhiteSpace(value))
        {
            return false;
        }

        var text = value.Trim();
        if (text.Length > 0 && (text[0] == 'v' || text[0] == 'V'))
        {
            text = text[1..];
        }

        var components = text.Split('.');
        if (components.Length != 3 ||
            !TryParseComponent(components[0], out var major) ||
            !TryParseComponent(components[1], out var minor) ||
            !TryParseComponent(components[2], out var patch))
        {
            return false;
        }

        version = new ProductVersion(major, minor, patch);
        return true;
    }

    public override string ToString() => string.Create(
        CultureInfo.InvariantCulture,
        $"{Major}.{Minor}.{Patch}");

    public static bool operator <(ProductVersion left, ProductVersion right) => left.CompareTo(right) < 0;
    public static bool operator >(ProductVersion left, ProductVersion right) => left.CompareTo(right) > 0;
    public static bool operator <=(ProductVersion left, ProductVersion right) => left.CompareTo(right) <= 0;
    public static bool operator >=(ProductVersion left, ProductVersion right) => left.CompareTo(right) >= 0;

    private static bool TryParseComponent(string text, out int value)
    {
        value = 0;
        if (text.Length == 0 || (text.Length > 1 && text[0] == '0'))
        {
            return false;
        }

        foreach (var character in text)
        {
            if (character is < '0' or > '9')
            {
                return false;
            }
        }

        return int.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out value);
    }
}

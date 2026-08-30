using System.Text.RegularExpressions;

namespace Menu11.Shared.Updates;

public static partial class UpdateChecksum
{
    public static bool TryParse(string? text, string expectedFileName, out byte[] hash)
    {
        hash = [];
        if (string.IsNullOrWhiteSpace(text) || string.IsNullOrWhiteSpace(expectedFileName))
        {
            return false;
        }

        var match = ChecksumLineRegex().Match(text);
        if (!match.Success ||
            !match.Groups["file"].Value.Equals(expectedFileName, StringComparison.Ordinal))
        {
            return false;
        }

        try
        {
            hash = Convert.FromHexString(match.Groups["hash"].Value);
            return hash.Length == 32;
        }
        catch (FormatException)
        {
            hash = [];
            return false;
        }
    }

    [GeneratedRegex(
        @"\A\s*(?<hash>[0-9A-Fa-f]{64})\s+\*?(?<file>[^\r\n]+?)\s*\z",
        RegexOptions.CultureInvariant)]
    private static partial Regex ChecksumLineRegex();
}

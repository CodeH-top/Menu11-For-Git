using System.Diagnostics;
using System.Security;
using Microsoft.Win32;

namespace Menu11.Shared.Git;

public sealed class GitDetectionService
{
    private const string GitForWindowsRegistryKey = @"Software\GitForWindows";

    public GitInstallation? Detect(string? userSelectedPath = null)
    {
        var registryLocations = new[]
        {
            (RegistryHive.LocalMachine, RegistryView.Registry64, GitDetectionSource.MachineRegistry64),
            (RegistryHive.LocalMachine, RegistryView.Registry32, GitDetectionSource.MachineRegistry32),
            (RegistryHive.CurrentUser, RegistryView.Registry64, GitDetectionSource.UserRegistry64),
            (RegistryHive.CurrentUser, RegistryView.Registry32, GitDetectionSource.UserRegistry32),
        };

        foreach (var (hive, view, source) in registryLocations)
        {
            var installPath = ReadRegistryInstallPath(hive, view);
            var installation = InspectCandidate(installPath, source);
            if (installation is not null)
            {
                return installation;
            }
        }

        var pathValue = Environment.GetEnvironmentVariable("PATH");
        if (!string.IsNullOrWhiteSpace(pathValue))
        {
            foreach (var directory in pathValue.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries))
            {
                var installation = InspectCandidate(
                    Path.Combine(directory.Trim().Trim('"'), "git.exe"),
                    GitDetectionSource.PathEnvironment);
                if (installation is not null)
                {
                    return installation;
                }
            }
        }

        foreach (var root in CommonInstallRoots())
        {
            var installation = InspectCandidate(root, GitDetectionSource.CommonLocation);
            if (installation is not null)
            {
                return installation;
            }
        }

        return InspectCandidate(userSelectedPath, GitDetectionSource.UserSelected);
    }

    public GitInstallation? InspectCandidate(string? candidate, GitDetectionSource source)
    {
        if (string.IsNullOrWhiteSpace(candidate))
        {
            return null;
        }

        try
        {
            var expanded = Environment.ExpandEnvironmentVariables(candidate.Trim().Trim('"'));
            var normalizedCandidate = Path.GetFullPath(expanded);
            string installRoot;
            string? explicitGit = null;

            if (File.Exists(normalizedCandidate))
            {
                if (!Path.GetFileName(normalizedCandidate).Equals("git.exe", StringComparison.OrdinalIgnoreCase))
                {
                    return null;
                }

                explicitGit = normalizedCandidate;
                installRoot = Path.GetDirectoryName(normalizedCandidate)!;
                if (IsGitBinaryDirectory(installRoot))
                {
                    installRoot = Directory.GetParent(installRoot)?.FullName ?? installRoot;
                }
            }
            else if (Directory.Exists(normalizedCandidate))
            {
                installRoot = normalizedCandidate;
                if (IsGitBinaryDirectory(installRoot))
                {
                    explicitGit = Path.Combine(installRoot, "git.exe");
                    installRoot = Directory.GetParent(installRoot)?.FullName ?? installRoot;
                }
            }
            else
            {
                return null;
            }

            installRoot = Path.GetFullPath(installRoot);
            var gitExecutable = FirstExistingFile(
                explicitGit,
                Path.Combine(installRoot, "cmd", "git.exe"),
                Path.Combine(installRoot, "bin", "git.exe"),
                Path.Combine(installRoot, "git.exe"));
            if (gitExecutable is null)
            {
                return null;
            }

            return new GitInstallation(
                installRoot,
                gitExecutable,
                FirstExistingFile(Path.Combine(installRoot, "git-bash.exe")),
                FirstExistingFile(Path.Combine(installRoot, "bin", "bash.exe")),
                FirstExistingFile(
                    Path.Combine(installRoot, "cmd", "git-gui.exe"),
                    Path.Combine(installRoot, "bin", "git-gui.exe"),
                    Path.Combine(installRoot, "git-gui.exe")),
                ReadProductVersion(gitExecutable),
                source);
        }
        catch (Exception exception) when (
            exception is ArgumentException or IOException or SecurityException or UnauthorizedAccessException)
        {
            return null;
        }
    }

    private static string? ReadRegistryInstallPath(RegistryHive hive, RegistryView view)
    {
        try
        {
            using var baseKey = RegistryKey.OpenBaseKey(hive, view);
            using var key = baseKey.OpenSubKey(GitForWindowsRegistryKey, writable: false);
            return key?.GetValue("InstallPath", null, RegistryValueOptions.DoNotExpandEnvironmentNames) as string;
        }
        catch (Exception exception) when (
            exception is ArgumentException or IOException or SecurityException or UnauthorizedAccessException)
        {
            return null;
        }
    }

    private static IEnumerable<string> CommonInstallRoots()
    {
        var roots = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var variable in new[] { "ProgramW6432", "ProgramFiles", "ProgramFiles(x86)" })
        {
            var basePath = Environment.GetEnvironmentVariable(variable);
            if (!string.IsNullOrWhiteSpace(basePath))
            {
                roots.Add(Path.Combine(basePath, "Git"));
            }
        }

        var localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        if (!string.IsNullOrWhiteSpace(localAppData))
        {
            roots.Add(Path.Combine(localAppData, "Programs", "Git"));
        }

        return roots;
    }

    private static bool IsGitBinaryDirectory(string path)
    {
        var name = Path.GetFileName(Path.TrimEndingDirectorySeparator(path));
        return name.Equals("cmd", StringComparison.OrdinalIgnoreCase) ||
            name.Equals("bin", StringComparison.OrdinalIgnoreCase);
    }

    private static string? FirstExistingFile(params string?[] candidates)
    {
        foreach (var candidate in candidates)
        {
            if (!string.IsNullOrWhiteSpace(candidate) && File.Exists(candidate))
            {
                return Path.GetFullPath(candidate);
            }
        }

        return null;
    }

    private static string ReadProductVersion(string executable)
    {
        try
        {
            var versionInfo = FileVersionInfo.GetVersionInfo(executable);
            return versionInfo.ProductVersion ?? versionInfo.FileVersion ?? string.Empty;
        }
        catch (Exception exception) when (exception is FileNotFoundException or IOException or SecurityException)
        {
            return string.Empty;
        }
    }
}

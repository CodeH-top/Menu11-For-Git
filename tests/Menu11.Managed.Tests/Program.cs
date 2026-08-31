using Menu11.Shared.Git;
using Menu11.Shared.Settings;
using Menu11.Shared.Updates;
using System.Net;
using System.Security.Cryptography;
using System.Text;

var failures = 0;

void Expect(bool condition, string message)
{
    if (condition)
    {
        Console.WriteLine($"[PASS] {message}");
        return;
    }

    failures++;
    Console.Error.WriteLine($"[FAIL] {message}");
}

async Task ExpectThrowsAsync<TException>(Func<Task> action, string message)
    where TException : Exception
{
    try
    {
        await action();
        Expect(false, message);
    }
    catch (TException)
    {
        Expect(true, message);
    }
}

var temporaryRoot = Path.Combine(Path.GetTempPath(), $"Menu11ManagedGitDetectionTests-{Environment.ProcessId}");
try
{
    Directory.Delete(temporaryRoot, recursive: true);
}
catch (DirectoryNotFoundException)
{
}

try
{
    var gitRoot = Path.Combine(temporaryRoot, "Portable Git 测试");
    var gitExecutable = Path.Combine(gitRoot, "cmd", "git.exe");
    var gitBashExecutable = Path.Combine(gitRoot, "git-bash.exe");
    var bashExecutable = Path.Combine(gitRoot, "bin", "bash.exe");
    var guiExecutable = Path.Combine(gitRoot, "cmd", "git-gui.exe");
    Directory.CreateDirectory(Path.GetDirectoryName(gitExecutable)!);
    Directory.CreateDirectory(Path.GetDirectoryName(bashExecutable)!);
    File.WriteAllBytes(gitExecutable, [0]);
    File.WriteAllBytes(gitBashExecutable, [0]);
    File.WriteAllBytes(bashExecutable, [0]);
    File.WriteAllBytes(guiExecutable, [0]);

    var service = new GitDetectionService();
    var fromRoot = service.InspectCandidate(gitRoot, GitDetectionSource.UserSelected);
    Expect(fromRoot is not null, "accepts a Git for Windows installation root");
    if (fromRoot is not null)
    {
        Expect(fromRoot.GitExecutable == Path.GetFullPath(gitExecutable), "prefers cmd\\git.exe");
        Expect(fromRoot.GitBashExecutable == Path.GetFullPath(gitBashExecutable), "finds git-bash.exe");
        Expect(fromRoot.BashExecutable == Path.GetFullPath(bashExecutable), "finds bin\\bash.exe");
        Expect(fromRoot.GuiExecutable == Path.GetFullPath(guiExecutable), "finds cmd\\git-gui.exe");
    }

    var fromExecutable = service.InspectCandidate(gitExecutable, GitDetectionSource.PathEnvironment);
    Expect(fromExecutable?.InstallRoot == Path.GetFullPath(gitRoot),
        "derives the install root from an explicit cmd\\git.exe path");
    Expect(service.InspectCandidate(Path.Combine(temporaryRoot, "NotGit"), GitDetectionSource.UserSelected) is null,
        "rejects a missing candidate");

    var detected = service.Detect();
    if (detected is null)
    {
        Console.WriteLine("[SKIP] Git for Windows is not installed on this test machine.");
    }
    else
    {
        Expect(File.Exists(detected.GitExecutable), "system detection returns an existing git.exe");
        Console.WriteLine($"[INFO] Git root: {detected.InstallRoot}");
        Console.WriteLine($"[INFO] Git version: {detected.Version}");
        Console.WriteLine($"[INFO] Detection source: {detected.Source}");
    }

    var settingsSubkey = $@"Software\Menu11ForGit\Tests\Managed-{Environment.ProcessId}";
    var settingsStore = new RegistrySettingsStore(settingsSubkey);
    settingsStore.Reset();
    var defaults = settingsStore.Load();
    Expect(defaults.Enabled && defaults.ShowGitBash && defaults.ShowGitGui,
        "uses enabled General defaults when the settings key is absent");
    Expect(defaults.EnabledCommands == GitCommandDefaults.Enabled,
        "uses the specified default Git command mask");
    Expect(defaults.Language == Menu11Language.System,
        "uses the system language by default");
    Expect(defaults.AutomaticallyCheckForUpdates,
        "checks for updates on application launch by default");
    var defaultPackages = ContextMenuPackageSelection.FromSettings(defaults);
    Expect(defaultPackages.GitBash && defaultPackages.GitGui && defaultPackages.GitCommands,
        "registers all context-menu packages for the default settings");

    var disabledPackages = ContextMenuPackageSelection.FromSettings(defaults with { Enabled = false });
    Expect(!disabledPackages.GitBash && !disabledPackages.GitGui && !disabledPackages.GitCommands,
        "unregisters every context-menu package when Menu11 is disabled");

    var noGitGuiPackage = ContextMenuPackageSelection.FromSettings(defaults with { ShowGitGui = false });
    Expect(noGitGuiPackage.GitBash && !noGitGuiPackage.GitGui && noGitGuiPackage.GitCommands,
        "unregisters the Git GUI package when its top-level command is disabled");

    var emptyGitMenuPackages = ContextMenuPackageSelection.FromSettings(defaults with
    {
        EnabledCommands = GitCommands.None,
        ShowSettingsCommand = false,
    });
    Expect(!emptyGitMenuPackages.GitCommands,
        "unregisters the Git package when its submenu has no enabled commands");

    var custom = new Menu11Settings
    {
        Enabled = false,
        ShowGitBash = false,
        ShowGitGui = true,
        EnabledCommands = GitCommands.Status | GitCommands.FileRestore,
        ShowSettingsCommand = false,
        ShowIcons = false,
        AutomaticallyCheckForUpdates = false,
        GitPath = @"C:\工具\PortableGit",
        Language = Menu11Language.SimplifiedChinese,
    };
    settingsStore.Save(custom);
    var loaded = settingsStore.Load();
    Expect(!loaded.Enabled && !loaded.ShowGitBash && loaded.ShowGitGui,
        "round-trips managed boolean settings");
    Expect(loaded.IsCommandEnabled(GitCommands.Status) &&
        loaded.IsCommandEnabled(GitCommands.FileRestore) &&
        !loaded.IsCommandEnabled(GitCommands.Pull),
        "round-trips the managed command mask");
    Expect(loaded.GitPath == custom.GitPath, "round-trips a Unicode Git path");
    Expect(loaded.Language == Menu11Language.SimplifiedChinese,
        "round-trips the managed language preference");
    Expect(!loaded.AutomaticallyCheckForUpdates,
        "round-trips the automatic update preference");
    settingsStore.Reset();
    Expect(settingsStore.Load().EnabledCommands == GitCommandDefaults.Enabled,
        "restores managed defaults after key removal");

    Expect(ProductVersion.TryParse("0.1.11", out var patchVersion) &&
        patchVersion == new ProductVersion(0, 1, 11),
        "accepts a three-component product version");
    Expect(ProductVersion.TryParse("v0.1.1", out var taggedVersion) &&
        taggedVersion.ToString() == "0.1.1",
        "accepts a normalized GitHub release tag");
    Expect(!ProductVersion.TryParse("0.1.1.0", out _),
        "rejects a four-component user-visible version");
    Expect(!ProductVersion.TryParse("v0.01.1", out _),
        "rejects a release tag with a leading zero");
    Expect(new ProductVersion(0, 1, 11) > new ProductVersion(0, 1, 2),
        "compares numeric patch versions instead of comparing strings");

    var installerPayload = Encoding.UTF8.GetBytes("verified Menu11 installer payload");
    var installerHash = Convert.ToHexString(SHA256.HashData(installerPayload));
    const string installerName = "Menu11ForGitSetup-0.1.3-x64.exe";
    var checksumText = $"{installerHash} *{installerName}\n";
    Expect(UpdateChecksum.TryParse(checksumText, installerName, out var parsedHash) &&
        parsedHash.SequenceEqual(SHA256.HashData(installerPayload)),
        "parses a release SHA-256 manifest for the exact installer name");
    Expect(!UpdateChecksum.TryParse(checksumText, "Menu11ForGitSetup-0.1.4-x64.exe", out _),
        "rejects a checksum manifest for another installer");

    var releaseJson = $$"""
        {
          "tag_name": "v0.1.3",
          "html_url": "https://github.com/CodeH-top/Menu11-For-Git/releases/tag/v0.1.3",
          "draft": false,
          "prerelease": false,
          "assets": [
            {
              "name": "{{installerName}}",
              "browser_download_url": "https://github.com/CodeH-top/Menu11-For-Git/releases/download/v0.1.3/{{installerName}}",
              "size": {{installerPayload.Length}}
            },
            {
              "name": "{{installerName}}.sha256",
              "browser_download_url": "https://github.com/CodeH-top/Menu11-For-Git/releases/download/v0.1.3/{{installerName}}.sha256",
              "size": {{Encoding.UTF8.GetByteCount(checksumText)}}
            }
          ]
        }
        """;
    using var releaseHttpClient = new HttpClient(new FakeHttpMessageHandler(request =>
    {
        Expect(request.Headers.UserAgent.ToString().Contains("Menu11-for-Git-Updater", StringComparison.Ordinal),
            "identifies update requests to GitHub");
        return new HttpResponseMessage(HttpStatusCode.OK)
        {
            Content = new StringContent(releaseJson, Encoding.UTF8, "application/json"),
        };
    }));
    var releaseClient = new GitHubReleaseClient(releaseHttpClient);
    var updateCheck = await releaseClient.CheckForUpdatesAsync(new ProductVersion(0, 1, 2));
    Expect(updateCheck.AvailableRelease?.Version == new ProductVersion(0, 1, 3),
        "discovers a newer stable GitHub release");
    Expect(updateCheck.AvailableRelease?.InstallerFileName == installerName,
        "requires the versioned x64 Setup asset name");

    var downloadRoot = Path.Combine(temporaryRoot, "updates");
    using var downloadHttpClient = new HttpClient(new FakeHttpMessageHandler(request =>
    {
        var fileName = Uri.UnescapeDataString(request.RequestUri!.Segments[^1]);
        return new HttpResponseMessage(HttpStatusCode.OK)
        {
            Content = fileName.EndsWith(".sha256", StringComparison.Ordinal)
                ? new StringContent(checksumText, Encoding.ASCII)
                : new ByteArrayContent(installerPayload),
        };
    }));
    var downloader = new UpdatePackageDownloader(downloadHttpClient, downloadRoot);
    var downloadedInstaller = await downloader.DownloadAndVerifyAsync(updateCheck.AvailableRelease!);
    Expect(File.Exists(downloadedInstaller) &&
        File.ReadAllBytes(downloadedInstaller).SequenceEqual(installerPayload),
        "downloads an installer only after its SHA-256 manifest is verified");

    var corruptPayload = installerPayload.ToArray();
    corruptPayload[0] ^= 0x5A;
    using var corruptHttpClient = new HttpClient(new FakeHttpMessageHandler(request =>
    {
        var fileName = Uri.UnescapeDataString(request.RequestUri!.Segments[^1]);
        return new HttpResponseMessage(HttpStatusCode.OK)
        {
            Content = fileName.EndsWith(".sha256", StringComparison.Ordinal)
                ? new StringContent(checksumText, Encoding.ASCII)
                : new ByteArrayContent(corruptPayload),
        };
    }));
    var corruptDownloadRoot = Path.Combine(temporaryRoot, "corrupt-updates");
    var corruptDownloader = new UpdatePackageDownloader(corruptHttpClient, corruptDownloadRoot);
    await ExpectThrowsAsync<UpdateException>(
        () => corruptDownloader.DownloadAndVerifyAsync(updateCheck.AvailableRelease!),
        "rejects a downloaded installer whose SHA-256 does not match");
    Expect(!File.Exists(Path.Combine(corruptDownloadRoot, "0.1.2", installerName)),
        "does not retain an unverified installer as an installable file");
}
finally
{
    try
    {
        Directory.Delete(temporaryRoot, recursive: true);
    }
    catch (DirectoryNotFoundException)
    {
    }

    var settingsSubkey = $@"Software\Menu11ForGit\Tests\Managed-{Environment.ProcessId}";
    new RegistrySettingsStore(settingsSubkey).Reset();
}

return failures == 0 ? 0 : 1;

internal sealed class FakeHttpMessageHandler(
    Func<HttpRequestMessage, HttpResponseMessage> responder) : HttpMessageHandler
{
    protected override Task<HttpResponseMessage> SendAsync(
        HttpRequestMessage request,
        CancellationToken cancellationToken)
    {
        var response = responder(request);
        response.RequestMessage ??= request;
        return Task.FromResult(response);
    }
}

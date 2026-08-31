using System.Runtime.InteropServices;
using Menu11.Shared.Settings;
using Windows.ApplicationModel;
using Windows.Management.Deployment;

namespace Menu11.App.Services;

public sealed class ContextMenuRegistrationFailedEventArgs(Exception exception) : EventArgs
{
    public Exception Exception { get; } = exception;
}

public sealed class ContextMenuRegistrationService
{
    private const int AssociationChanged = 0x08000000;
    private const uint IdList = 0x0000;
    private const uint Flush = 0x1000;

    private static readonly PackageDefinition[] PackageDefinitions =
    [
        new(
            "Menu11ForGit.GitBash",
            Path.Combine("Package", "GitBash", "AppxManifest.xml"),
            static selection => selection.GitBash),
        new(
            "Menu11ForGit.GitGui",
            Path.Combine("Package", "GitGui", "AppxManifest.xml"),
            static selection => selection.GitGui),
        new(
            "Menu11ForGit.GitCommands",
            Path.Combine("Package", "GitCommands", "AppxManifest.xml"),
            static selection => selection.GitCommands),
    ];

    private readonly SemaphoreSlim _synchronizationGate = new(1, 1);
    private int _latestRequest;

    public event EventHandler<ContextMenuRegistrationFailedEventArgs>? SynchronizationFailed;

    public void RequestSynchronization(Menu11Settings settings)
    {
        ArgumentNullException.ThrowIfNull(settings);
        var request = Interlocked.Increment(ref _latestRequest);
        _ = SynchronizeRequestedAsync(settings, request);
    }

    public Task SynchronizeAsync(Menu11Settings settings, bool forceRegistration = false)
    {
        ArgumentNullException.ThrowIfNull(settings);
        var request = Interlocked.Increment(ref _latestRequest);
        return SynchronizeCoreAsync(
            settings,
            request,
            skipIfStale: false,
            forceRegistration: forceRegistration);
    }

    private async Task SynchronizeRequestedAsync(Menu11Settings settings, int request)
    {
        try
        {
            await SynchronizeCoreAsync(
                settings,
                request,
                skipIfStale: true,
                forceRegistration: false);
        }
        catch (Exception exception)
        {
            if (request == Volatile.Read(ref _latestRequest))
            {
                SynchronizationFailed?.Invoke(
                    this,
                    new ContextMenuRegistrationFailedEventArgs(exception));
            }
        }
    }

    private async Task SynchronizeCoreAsync(
        Menu11Settings settings,
        int request,
        bool skipIfStale,
        bool forceRegistration)
    {
        await _synchronizationGate.WaitAsync();
        try
        {
            if (skipIfStale && request != Volatile.Read(ref _latestRequest))
            {
                return;
            }

            var installDirectory = Path.GetFullPath(AppContext.BaseDirectory);
            var selection = ContextMenuPackageSelection.FromSettings(settings);
            var packageManager = new PackageManager();
            var managedPackageNames = PackageDefinitions
                .Select(definition => definition.Name)
                .ToHashSet(StringComparer.OrdinalIgnoreCase);
            var installedPackages = packageManager
                .FindPackagesForUser(string.Empty)
                .Where(package => managedPackageNames.Contains(package.Id.Name))
                .GroupBy(package => package.Id.Name, StringComparer.OrdinalIgnoreCase)
                .ToDictionary(group => group.Key, group => group.ToArray(), StringComparer.OrdinalIgnoreCase);

            foreach (var definition in PackageDefinitions)
            {
                var manifestPath = Path.GetFullPath(Path.Combine(installDirectory, definition.ManifestPath));
                var shouldRegister = definition.ShouldRegister(selection);
                installedPackages.TryGetValue(definition.Name, out var packages);
                packages ??= [];

                var reusablePackage = forceRegistration
                    ? null
                    : packages.FirstOrDefault(package =>
                        IsReusablePackage(package, Path.GetDirectoryName(manifestPath)!));
                foreach (var package in packages)
                {
                    if (!shouldRegister || !ReferenceEquals(package, reusablePackage))
                    {
                        await RemovePackageAsync(packageManager, package);
                    }
                }

                if (!shouldRegister || reusablePackage is not null)
                {
                    continue;
                }
                if (!File.Exists(manifestPath))
                {
                    throw new FileNotFoundException(
                        $"Context-menu package manifest was not found: {manifestPath}",
                        manifestPath);
                }

                var options = new RegisterPackageOptions
                {
                    DeveloperMode = true,
                    ExternalLocationUri = ToDirectoryUri(installDirectory),
                    ForceAppShutdown = true,
                    ForceTargetAppShutdown = true,
                };
                DeploymentResult result;
                try
                {
                    result = await packageManager.RegisterPackageByUriAsync(
                        new Uri(manifestPath),
                        options);
                }
                catch (Exception exception)
                {
                    throw CreateDeploymentException($"register {definition.Name}", exception);
                }
                ThrowIfDeploymentFailed(result, $"register {definition.Name}");
            }

            SHChangeNotify(AssociationChanged, IdList | Flush, 0, 0);
        }
        finally
        {
            _synchronizationGate.Release();
        }
    }

    private static bool IsReusablePackage(Package package, string expectedLocation)
    {
        try
        {
            return package.Status.VerifyIsOK() && string.Equals(
                Path.GetFullPath(package.InstalledLocation.Path).TrimEnd(Path.DirectorySeparatorChar),
                Path.GetFullPath(expectedLocation).TrimEnd(Path.DirectorySeparatorChar),
                StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            return false;
        }
    }

    private static async Task RemovePackageAsync(PackageManager packageManager, Package package)
    {
        DeploymentResult result;
        try
        {
            result = await packageManager.RemovePackageAsync(package.Id.FullName);
        }
        catch (Exception exception)
        {
            throw CreateDeploymentException($"remove {package.Id.Name}", exception);
        }
        ThrowIfDeploymentFailed(result, $"remove {package.Id.Name}");
    }

    private static void ThrowIfDeploymentFailed(DeploymentResult result, string action)
    {
        if (result.ExtendedErrorCode is not { HResult: < 0 } error)
        {
            return;
        }

        var details = string.IsNullOrWhiteSpace(result.ErrorText)
            ? error.Message
            : result.ErrorText;
        throw new InvalidOperationException($"Unable to {action}: {details}", error);
    }

    private static InvalidOperationException CreateDeploymentException(
        string action,
        Exception exception)
    {
        var details = string.IsNullOrWhiteSpace(exception.Message)
            ? $"HRESULT 0x{exception.HResult:X8}"
            : exception.Message;
        return new InvalidOperationException($"Unable to {action}: {details}", exception);
    }

    private static Uri ToDirectoryUri(string path)
    {
        return new Uri(Path.EndsInDirectorySeparator(path)
            ? path
            : path + Path.DirectorySeparatorChar);
    }

    [DllImport("shell32.dll", ExactSpelling = true)]
    private static extern void SHChangeNotify(
        int eventId,
        uint flags,
        nint item1,
        nint item2);

    private sealed record PackageDefinition(
        string Name,
        string ManifestPath,
        Func<ContextMenuPackageSelection, bool> ShouldRegister);
}

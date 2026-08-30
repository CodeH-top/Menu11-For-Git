namespace Menu11.Shared.Git;

public enum GitDetectionSource
{
    MachineRegistry64,
    MachineRegistry32,
    UserRegistry64,
    UserRegistry32,
    PathEnvironment,
    CommonLocation,
    UserSelected,
}

public sealed record GitInstallation(
    string InstallRoot,
    string GitExecutable,
    string? GitBashExecutable,
    string? BashExecutable,
    string? GuiExecutable,
    string Version,
    GitDetectionSource Source);

namespace Menu11.Shared.Settings;

[Flags]
public enum GitCommands : uint
{
    None = 0,
    Status = 1u << 0,
    Pull = 1u << 1,
    Fetch = 1u << 2,
    Push = 1u << 3,
    Commit = 1u << 4,
    RepositoryLog = 1u << 5,
    Branch = 1u << 6,
    Stash = 1u << 7,
    FileAdd = 1u << 8,
    FileDiff = 1u << 9,
    FileLog = 1u << 10,
    FileBlame = 1u << 11,
    FileRestore = 1u << 12,
    Clone = 1u << 13,
    Init = 1u << 14,
    All = 0x00007FFFu,
}

public static class GitCommandDefaults
{
    public const GitCommands Enabled =
        GitCommands.Status |
        GitCommands.Pull |
        GitCommands.Fetch |
        GitCommands.Push |
        GitCommands.Commit |
        GitCommands.FileAdd |
        GitCommands.FileDiff |
        GitCommands.Clone |
        GitCommands.Init;
}

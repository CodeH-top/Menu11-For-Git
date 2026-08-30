using System.Runtime.InteropServices;

namespace Menu11.App.Services;

public static class ExplorerIntegrationService
{
    private const uint AssociationChanged = 0x08000000;
    private const uint IdList = 0x0000;

    public static void Refresh()
    {
        SHChangeNotify(AssociationChanged, IdList, 0, 0);
    }

    [DllImport("shell32.dll")]
    private static extern void SHChangeNotify(uint eventId, uint flags, nint item1, nint item2);
}

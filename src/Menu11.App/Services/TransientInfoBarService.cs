using System.Runtime.CompilerServices;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml.Controls;

namespace Menu11.App.Services;

public static class TransientInfoBarService
{
    public static readonly TimeSpan DefaultDuration = TimeSpan.FromSeconds(3);

    private static readonly ConditionalWeakTable<InfoBar, DispatcherQueueTimer> Timers = new();

    public static void Show(
        InfoBar infoBar,
        string title,
        string message,
        InfoBarSeverity severity)
    {
        ArgumentNullException.ThrowIfNull(infoBar);

        infoBar.Title = title;
        infoBar.Message = message;
        infoBar.Severity = severity;
        infoBar.IsOpen = true;

        var timer = Timers.GetValue(infoBar, CreateTimer);
        timer.Stop();
        timer.Interval = DefaultDuration;
        timer.Start();
    }

    private static DispatcherQueueTimer CreateTimer(InfoBar infoBar)
    {
        var timer = infoBar.DispatcherQueue.CreateTimer();
        timer.IsRepeating = false;
        timer.Tick += (_, _) => infoBar.IsOpen = false;
        return timer;
    }
}

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Menu11.App.Views;
using System.Runtime.InteropServices;
using Windows.Graphics;

namespace Menu11.App;

public sealed partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        SetWindowIcon();
        ResizeForCurrentDisplay();
        Navigation.SelectedItem = Navigation.MenuItems[0];
        ContentFrame.Navigate(typeof(GeneralPage));
    }

    private void SetWindowIcon()
    {
        var iconPath = Path.Combine(AppContext.BaseDirectory, "Assets", "AppIcon.ico");
        if (File.Exists(iconPath))
        {
            AppWindow.SetIcon(iconPath);
        }
    }

    private void ResizeForCurrentDisplay()
    {
        var windowHandle = WinRT.Interop.WindowNative.GetWindowHandle(this);
        var scale = GetDpiForWindow(windowHandle) / 96.0;
        var displayArea = Microsoft.UI.Windowing.DisplayArea.GetFromWindowId(
            AppWindow.Id,
            Microsoft.UI.Windowing.DisplayAreaFallback.Primary);
        var margin = (int)Math.Round(40 * scale);
        var width = Math.Min(
            (int)Math.Round(940 * scale),
            Math.Max(1, displayArea.WorkArea.Width - margin));
        var height = Math.Min(
            (int)Math.Round(640 * scale),
            Math.Max(1, displayArea.WorkArea.Height - margin));
        AppWindow.Resize(new SizeInt32(width, height));
    }

    private void Navigation_ItemInvoked(NavigationView sender, NavigationViewItemInvokedEventArgs args)
    {
        if (args.InvokedItemContainer is not NavigationViewItem item)
        {
            return;
        }

        var pageType = item.Tag switch
        {
            "general" => typeof(GeneralPage),
            "git-menu" => typeof(GitMenuPage),
            "git-for-windows" => typeof(GitForWindowsPage),
            "about" => typeof(AboutPage),
            _ => typeof(GeneralPage),
        };

        if (ContentFrame.CurrentSourcePageType != pageType)
        {
            ContentFrame.Navigate(pageType);
        }
    }

    [DllImport("user32.dll")]
    private static extern uint GetDpiForWindow(nint windowHandle);
}

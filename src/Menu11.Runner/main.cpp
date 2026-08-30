#include "CommandDispatcher.h"
#include "Menu11.Shared/Localization.h"
#include "Menu11.Shared/RegistrySettings.h"
#include "Menu11.Shared/RunnerProtocol.h"

#include <windows.h>
#include <shellapi.h>

#include <string>
#include <string_view>
#include <vector>

#pragma comment(lib, "Shell32.lib")

namespace
{
    enum class runner_exit_code : int
    {
        success = 0,
        invalid_arguments = 2,
        unsupported_command = 3,
        execution_failed = 4,
    };

    [[nodiscard]] menu11::display_language runner_language() noexcept
    {
        return menu11::registry_settings_store().load().language;
    }

    [[nodiscard]] bool use_chinese(const menu11::display_language language) noexcept
    {
        return menu11::resolve_display_language(language) ==
            menu11::display_language::simplified_chinese;
    }

    [[nodiscard]] std::wstring_view translated(
        const menu11::display_language language,
        const std::wstring_view english,
        const std::wstring_view simplified_chinese) noexcept
    {
        return use_chinese(language) ? simplified_chinese : english;
    }

    void show_error(const std::wstring_view message) noexcept
    {
        const std::wstring text(message);
        MessageBoxW(
            nullptr,
            text.c_str(),
            L"Menu11 for Git",
            MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    }

    [[nodiscard]] std::wstring_view command_name(
        const menu11::runner_command command,
        const menu11::display_language language) noexcept
    {
        using menu11::localized_string;
        switch (command)
        {
        case menu11::runner_command::git_bash:
            return menu11::localize(localized_string::git_bash_here, language);
        case menu11::runner_command::git_gui:
            return menu11::localize(localized_string::git_gui_here, language);
        case menu11::runner_command::status:
            return menu11::localize(localized_string::status, language);
        case menu11::runner_command::pull:
            return menu11::localize(localized_string::pull, language);
        case menu11::runner_command::fetch:
            return menu11::localize(localized_string::fetch, language);
        case menu11::runner_command::push:
            return menu11::localize(localized_string::push, language);
        case menu11::runner_command::commit:
            return menu11::localize(localized_string::commit, language);
        case menu11::runner_command::repository_log:
        case menu11::runner_command::file_log:
            return menu11::localize(localized_string::log, language);
        case menu11::runner_command::branch:
            return menu11::localize(localized_string::branch, language);
        case menu11::runner_command::stash:
            return menu11::localize(localized_string::stash, language);
        case menu11::runner_command::file_add:
            return menu11::localize(localized_string::add, language);
        case menu11::runner_command::file_diff:
            return menu11::localize(localized_string::diff, language);
        case menu11::runner_command::file_blame:
            return menu11::localize(localized_string::blame, language);
        case menu11::runner_command::file_restore:
            return menu11::localize(localized_string::restore, language);
        case menu11::runner_command::clone:
            return menu11::localize(localized_string::clone, language);
        case menu11::runner_command::init:
            return menu11::localize(localized_string::init_repository, language);
        case menu11::runner_command::settings:
            return L"Menu11 for Git";
        }
        return L"Git";
    }

    std::wstring win32_error_text(const DWORD error)
    {
        wchar_t* system_message = nullptr;
        const DWORD length = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            error,
            0,
            reinterpret_cast<wchar_t*>(&system_message),
            0,
            nullptr);
        if (length == 0 || system_message == nullptr)
        {
            return L"Windows error " + std::to_wstring(error) + L".";
        }

        std::wstring message(system_message, length);
        LocalFree(system_message);
        while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n' || message.back() == L' '))
        {
            message.pop_back();
        }
        return message;
    }

    [[nodiscard]] bool confirm_restore(
        const std::size_t selection_count,
        const menu11::display_language language) noexcept
    {
        const auto title = translated(
            language,
            selection_count == 1 ? L"Restore selected file?" : L"Restore selected files?",
            L"恢复所选文件？");
        const auto body = translated(
            language,
            L"Uncommitted changes in the selected file(s) will be discarded.\n\nDo you want to continue?",
            L"所选文件中尚未提交的更改将被丢弃。\n\n是否继续？");
        const std::wstring text(body);
        const std::wstring caption(title);
        return MessageBoxW(
            nullptr,
            text.c_str(),
            caption.c_str(),
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 | MB_SETFOREGROUND) == IDYES;
    }

    void show_dispatch_error(
        const menu11::runner::dispatch_result& result,
        const menu11::runner_command command,
        const menu11::display_language language) noexcept
    {
        using menu11::runner::dispatch_error;
        switch (result.error)
        {
        case dispatch_error::menu_disabled:
            show_error(translated(
                language,
                L"The Menu11 context menu is disabled in Settings.",
                L"Menu11 右键菜单已在设置中禁用。"));
            break;
        case dispatch_error::command_disabled:
        {
            const auto suffix = translated(
                language,
                L" is disabled in Menu11 Settings.",
                L" 已在 Menu11 设置中禁用。");
            show_error(std::wstring(command_name(command, language)) + std::wstring(suffix));
            break;
        }
        case dispatch_error::git_not_found:
            show_error(translated(
                language,
                L"Git for Windows was not detected. Open Menu11 for Git to configure its location.",
                L"未检测到 Git for Windows。请打开 Menu11 for Git 配置其位置。"));
            break;
        case dispatch_error::git_bash_not_found:
            show_error(translated(
                language,
                L"git-bash.exe was not found in the detected Git for Windows installation.",
                L"在检测到的 Git for Windows 安装中找不到 git-bash.exe。"));
            break;
        case dispatch_error::bash_not_found:
            show_error(translated(
                language,
                L"bin\\bash.exe was not found in the detected Git for Windows installation. Repair or reinstall Git for Windows.",
                L"在检测到的 Git for Windows 安装中找不到 bin\\bash.exe。请修复或重新安装 Git for Windows。"));
            break;
        case dispatch_error::git_gui_not_found:
            show_error(translated(
                language,
                L"git-gui.exe was not found in the detected Git for Windows installation.",
                L"在检测到的 Git for Windows 安装中找不到 git-gui.exe。"));
            break;
        case dispatch_error::invalid_selection:
            show_error(translated(
                language,
                L"The selected file or folder is no longer available.",
                L"所选文件或文件夹已不可用。"));
            break;
        case dispatch_error::repository_required:
            show_error(translated(
                language,
                L"This command requires a folder inside a Git repository.",
                L"此命令需要选择 Git 仓库中的文件夹。"));
            break;
        case dispatch_error::file_selection_required:
            show_error(translated(
                language,
                L"This command requires file selections from the same Git repository.",
                L"此命令需要选择同一个 Git 仓库中的文件。"));
            break;
        case dispatch_error::non_repository_directory_required:
            show_error(translated(
                language,
                L"This command requires a folder that is not already inside a Git repository.",
                L"此命令需要选择尚未位于 Git 仓库中的文件夹。"));
            break;
        case dispatch_error::launch_failed:
        {
            const auto prefix = translated(
                language,
                L"Menu11 could not start ",
                L"Menu11 无法启动 ");
            const auto message = std::wstring(prefix) +
                std::wstring(command_name(command, language)) + L".\n\n" +
                win32_error_text(result.win32_error);
            show_error(message);
            break;
        }
        case dispatch_error::unsupported_command:
            show_error(translated(
                language,
                L"This Menu11 command is not available in the current build.",
                L"当前版本不支持此 Menu11 命令。"));
            break;
        case dispatch_error::confirmation_required:
        case dispatch_error::none:
            break;
        }
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    const auto language = runner_language();
    int argument_count = 0;
    wchar_t** argument_values = CommandLineToArgvW(GetCommandLineW(), &argument_count);
    if (argument_values == nullptr || argument_count < 1)
    {
        show_error(translated(
            language,
            L"Menu11 could not read the requested command.",
            L"Menu11 无法读取请求的命令。"));
        return static_cast<int>(runner_exit_code::invalid_arguments);
    }

    std::vector<std::wstring_view> arguments;
    arguments.reserve(static_cast<std::size_t>(argument_count - 1));
    for (int index = 1; index < argument_count; ++index)
    {
        arguments.emplace_back(argument_values[index]);
    }

    const auto parsed = menu11::parse_runner_arguments(arguments);
    LocalFree(argument_values);
    if (!parsed)
    {
        show_error(translated(
            language,
            L"Menu11 could not understand the requested command.",
            L"Menu11 无法识别请求的命令。"));
        return static_cast<int>(runner_exit_code::invalid_arguments);
    }

    auto result = menu11::runner::dispatch(*parsed.request);
    if (result.error == menu11::runner::dispatch_error::confirmation_required)
    {
        if (!confirm_restore(parsed.request->selections.size(), language))
        {
            return static_cast<int>(runner_exit_code::success);
        }
        result = menu11::runner::dispatch(*parsed.request, true);
    }

    if (!result)
    {
        show_dispatch_error(result, parsed.request->command, language);
        return result.error == menu11::runner::dispatch_error::unsupported_command
            ? static_cast<int>(runner_exit_code::unsupported_command)
            : static_cast<int>(runner_exit_code::execution_failed);
    }

    return static_cast<int>(runner_exit_code::success);
}

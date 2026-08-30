#include "ShellLauncher.h"

#include "Menu11.Shared/ProcessLauncher.h"
#include "Menu11.Shared/ProductInfo.h"
#include "Module.h"

#include <windows.h>

#include <vector>

namespace menu11::shell
{
    namespace
    {
        [[nodiscard]] std::filesystem::path module_directory() noexcept
        {
            try
            {
                std::vector<wchar_t> buffer(512, L'\0');
                for (;;)
                {
                    const DWORD length = GetModuleFileNameW(
                        module_instance(),
                        buffer.data(),
                        static_cast<DWORD>(buffer.size()));
                    if (length == 0)
                    {
                        return {};
                    }
                    if (length < buffer.size() - 1)
                    {
                        return std::filesystem::path(buffer.data()).parent_path();
                    }
                    if (buffer.size() >= 32768)
                    {
                        return {};
                    }
                    buffer.resize(buffer.size() * 2, L'\0');
                }
            }
            catch (...)
            {
                return {};
            }
        }
    }

    HRESULT launch_runner(
        const runner_command command,
        const std::span<const std::filesystem::path> selections) noexcept
    {
        try
        {
            const auto directory = module_directory();
            if (directory.empty())
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }

            const auto runner = directory / product::runner_executable;
            if (GetFileAttributesW(runner.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }

            process_launch_options options{
                .executable = runner,
                .arguments = {L"--command", std::wstring(runner_command_token(command))},
                .working_directory = directory,
                .show_window = SW_SHOWNORMAL,
                .wait_for_exit = false,
            };
            for (const auto& selection : selections)
            {
                options.arguments.emplace_back(L"--path");
                options.arguments.emplace_back(selection.wstring());
            }

            const auto result = launch_process(options);
            return result.started() ? S_OK : HRESULT_FROM_WIN32(result.error);
        }
        catch (...)
        {
            return E_OUTOFMEMORY;
        }
    }
}

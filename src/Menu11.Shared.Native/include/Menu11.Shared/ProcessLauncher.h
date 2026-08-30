#pragma once

#include <windows.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace menu11
{
    struct process_launch_options
    {
        std::filesystem::path executable;
        std::vector<std::wstring> arguments;
        std::filesystem::path working_directory;
        DWORD creation_flags = 0;
        WORD show_window = SW_SHOWNORMAL;
        bool wait_for_exit = false;
    };

    struct process_launch_result
    {
        DWORD error = ERROR_SUCCESS;
        DWORD process_id = 0;
        std::optional<DWORD> exit_code;

        [[nodiscard]] bool started() const noexcept
        {
            return error == ERROR_SUCCESS && process_id != 0;
        }
    };

    [[nodiscard]] process_launch_result launch_process(
        const process_launch_options& options) noexcept;

    [[nodiscard]] std::filesystem::path current_process_directory() noexcept;
}

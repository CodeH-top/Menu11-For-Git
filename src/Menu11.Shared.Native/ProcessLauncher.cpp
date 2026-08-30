#include "Menu11.Shared/ProcessLauncher.h"

#include "Menu11.Shared/WindowsCommandLine.h"

#include <limits>

namespace menu11
{
    namespace
    {
        constexpr std::size_t maximum_command_line_characters = 32767;

        class process_information final
        {
        public:
            PROCESS_INFORMATION value{};

            ~process_information()
            {
                if (value.hThread != nullptr)
                {
                    CloseHandle(value.hThread);
                }
                if (value.hProcess != nullptr)
                {
                    CloseHandle(value.hProcess);
                }
            }

            process_information(const process_information&) = delete;
            process_information& operator=(const process_information&) = delete;
            process_information() = default;
        };
    }

    process_launch_result launch_process(const process_launch_options& options) noexcept
    {
        try
        {
            if (options.executable.empty())
            {
                return {.error = ERROR_INVALID_PARAMETER};
            }

            auto command_line = build_windows_command_line(options.executable, options.arguments);
            if (command_line.size() + 1 > maximum_command_line_characters)
            {
                return {.error = ERROR_BAD_LENGTH};
            }

            STARTUPINFOW startup_info{};
            startup_info.cb = sizeof(startup_info);
            startup_info.dwFlags = STARTF_USESHOWWINDOW;
            startup_info.wShowWindow = options.show_window;

            process_information process;
            const std::wstring executable = options.executable.wstring();
            const std::wstring working_directory = options.working_directory.empty()
                ? std::wstring{}
                : options.working_directory.wstring();
            const BOOL created = CreateProcessW(
                executable.c_str(),
                command_line.data(),
                nullptr,
                nullptr,
                FALSE,
                options.creation_flags | CREATE_UNICODE_ENVIRONMENT,
                nullptr,
                working_directory.empty() ? nullptr : working_directory.c_str(),
                &startup_info,
                &process.value);
            if (!created)
            {
                return {.error = GetLastError()};
            }

            process_launch_result result{
                .error = ERROR_SUCCESS,
                .process_id = process.value.dwProcessId,
            };
            if (!options.wait_for_exit)
            {
                return result;
            }

            const DWORD wait_result = WaitForSingleObject(process.value.hProcess, INFINITE);
            if (wait_result != WAIT_OBJECT_0)
            {
                result.error = wait_result == WAIT_FAILED ? GetLastError() : ERROR_GEN_FAILURE;
                return result;
            }

            DWORD exit_code = 0;
            if (!GetExitCodeProcess(process.value.hProcess, &exit_code))
            {
                result.error = GetLastError();
                return result;
            }
            result.exit_code = exit_code;
            return result;
        }
        catch (...)
        {
            return {.error = ERROR_NOT_ENOUGH_MEMORY};
        }
    }

    std::filesystem::path current_process_directory() noexcept
    {
        try
        {
            std::vector<wchar_t> buffer(512, L'\0');
            for (;;)
            {
                const DWORD length = GetModuleFileNameW(
                    nullptr,
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

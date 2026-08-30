#include "Menu11.Shared/GitRepositoryCommands.h"

#include "Menu11.Shared/RepositoryDetector.h"

#include <windows.h>

#include <string>
#include <string_view>
#include <vector>

namespace menu11::git
{
    namespace
    {
        [[nodiscard]] std::wstring_view repository_script(const runner_command command) noexcept
        {
            switch (command)
            {
            case runner_command::status:
                return L"git status; menu11_result=$?; printf '\n\nMenu11: press Enter to close... '; "
                    L"read -r; exit $menu11_result";
            case runner_command::pull:
                return L"git pull; menu11_result=$?; printf '\n\nMenu11: press Enter to close... '; "
                    L"read -r; exit $menu11_result";
            case runner_command::fetch:
                return L"git fetch; menu11_result=$?; printf '\n\nMenu11: press Enter to close... '; "
                    L"read -r; exit $menu11_result";
            case runner_command::push:
                return L"git push; menu11_result=$?; printf '\n\nMenu11: press Enter to close... '; "
                    L"read -r; exit $menu11_result";
            case runner_command::repository_log:
                return L"git log --graph --decorate --oneline --all; menu11_result=$?; "
                    L"printf '\n\nMenu11: press Enter to close... '; read -r; exit $menu11_result";
            case runner_command::branch:
                return L"git branch --all; menu11_result=$?; printf '\n\nMenu11: press Enter to close... '; "
                    L"read -r; exit $menu11_result";
            case runner_command::stash:
                return L"git stash push; menu11_result=$?; printf '\n\nMenu11: press Enter to close... '; "
                    L"read -r; exit $menu11_result";
            default:
                return {};
            }
        }

        [[nodiscard]] std::wstring_view file_script(const runner_command command) noexcept
        {
            switch (command)
            {
            case runner_command::file_add:
                return L"git add -- \"$@\"; menu11_result=$?; "
                    L"printf '\n\nMenu11: press Enter to close... '; read -r; exit $menu11_result";
            case runner_command::file_diff:
                return L"git diff -- \"$1\"; menu11_result=$?; "
                    L"printf '\n\nMenu11: press Enter to close... '; read -r; exit $menu11_result";
            case runner_command::file_log:
                return L"git log --follow -- \"$1\"; menu11_result=$?; "
                    L"printf '\n\nMenu11: press Enter to close... '; read -r; exit $menu11_result";
            case runner_command::file_blame:
                return L"git blame -- \"$1\"; menu11_result=$?; "
                    L"printf '\n\nMenu11: press Enter to close... '; read -r; exit $menu11_result";
            case runner_command::file_restore:
                return L"git restore -- \"$@\"; menu11_result=$?; "
                    L"printf '\n\nMenu11: press Enter to close... '; read -r; exit $menu11_result";
            default:
                return {};
            }
        }

        [[nodiscard]] std::wstring_view directory_script(const runner_command command) noexcept
        {
            switch (command)
            {
            case runner_command::clone:
                return L"printf 'Repository URL: '; IFS= read -r menu11_url; "
                    L"if [ -z \"$menu11_url\" ]; then printf '\nClone cancelled.\n'; "
                    L"menu11_result=1; else printf 'Target directory (leave blank for default): '; "
                    L"IFS= read -r menu11_target; if [ -z \"$menu11_target\" ]; then "
                    L"git clone -- \"$menu11_url\"; else git clone -- \"$menu11_url\" \"$menu11_target\"; "
                    L"fi; menu11_result=$?; fi; printf '\n\nMenu11: press Enter to close... '; "
                    L"read -r; exit $menu11_result";
            case runner_command::init:
                return L"git init; menu11_result=$?; printf '\n\nMenu11: press Enter to close... '; "
                    L"read -r; exit $menu11_result";
            default:
                return {};
            }
        }

        [[nodiscard]] bool bash_is_available(const installation& git_installation) noexcept
        {
            return git_installation.has_bash() &&
                GetFileAttributesW(git_installation.bash_executable.c_str()) != INVALID_FILE_ATTRIBUTES;
        }

        [[nodiscard]] bool paths_equal(
            const std::filesystem::path& left,
            const std::filesystem::path& right) noexcept
        {
            const auto& left_text = left.native();
            const auto& right_text = right.native();
            return CompareStringOrdinal(
                left_text.c_str(), static_cast<int>(left_text.size()),
                right_text.c_str(), static_cast<int>(right_text.size()),
                TRUE) == CSTR_EQUAL;
        }
    }

    repository_launch_plan create_repository_launch_plan(
        const installation& git_installation,
        const runner_command command,
        const std::filesystem::path& selected_path) noexcept
    {
        try
        {
            const auto context = inspect_repository_context(selected_path);
            if (!context.selection_exists || context.selection_is_file || !context.repository_root)
            {
                return {.error = repository_plan_error::invalid_selection};
            }

            if (command == runner_command::commit)
            {
                if (!git_installation.has_gui() ||
                    GetFileAttributesW(git_installation.gui_executable.c_str()) == INVALID_FILE_ATTRIBUTES)
                {
                    return {.error = repository_plan_error::executable_unavailable};
                }

                return {
                    .error = repository_plan_error::none,
                    .options = process_launch_options{
                        .executable = git_installation.gui_executable,
                        .arguments = {L"citool"},
                        .working_directory = *context.repository_root,
                        .show_window = SW_SHOWNORMAL,
                        .wait_for_exit = false,
                    },
                };
            }

            const auto script = repository_script(command);
            if (script.empty())
            {
                return {.error = repository_plan_error::unsupported_command};
            }
            if (!bash_is_available(git_installation))
            {
                return {.error = repository_plan_error::executable_unavailable};
            }

            return {
                .error = repository_plan_error::none,
                .options = process_launch_options{
                    .executable = git_installation.bash_executable,
                    .arguments = {
                        L"--login",
                        L"-c",
                        std::wstring(script),
                        L"menu11",
                    },
                    .working_directory = *context.repository_root,
                    .creation_flags = CREATE_NEW_CONSOLE,
                    .show_window = SW_SHOWNORMAL,
                    .wait_for_exit = false,
                },
            };
        }
        catch (...)
        {
            return {.error = repository_plan_error::invalid_selection};
        }
    }

    file_launch_plan create_file_launch_plan(
        const installation& git_installation,
        const runner_command command,
        const std::span<const std::filesystem::path> selected_paths) noexcept
    {
        try
        {
            const auto script = file_script(command);
            if (script.empty())
            {
                return {.error = file_plan_error::unsupported_command};
            }
            if (selected_paths.empty())
            {
                return {.error = file_plan_error::invalid_selection};
            }

            const bool requires_single_selection = command == runner_command::file_diff ||
                command == runner_command::file_log || command == runner_command::file_blame;
            if (requires_single_selection && selected_paths.size() != 1)
            {
                return {.error = file_plan_error::invalid_selection};
            }

            std::optional<std::filesystem::path> repository_root;
            std::vector<std::filesystem::path> normalized_paths;
            normalized_paths.reserve(selected_paths.size());
            for (const auto& path : selected_paths)
            {
                const auto context = inspect_repository_context(path);
                if (!context.selection_exists || !context.selection_is_file || !context.repository_root)
                {
                    return {.error = file_plan_error::invalid_selection};
                }
                if (repository_root && !paths_equal(*repository_root, *context.repository_root))
                {
                    return {.error = file_plan_error::invalid_selection};
                }
                repository_root = context.repository_root;
                normalized_paths.push_back(context.selected_path);
            }
            if (!repository_root)
            {
                return {.error = file_plan_error::invalid_selection};
            }
            if (!bash_is_available(git_installation))
            {
                return {.error = file_plan_error::executable_unavailable};
            }

            process_launch_options options{
                .executable = git_installation.bash_executable,
                .arguments = {
                    L"--login",
                    L"-c",
                    std::wstring(script),
                    L"menu11",
                },
                .working_directory = *repository_root,
                .creation_flags = CREATE_NEW_CONSOLE,
                .show_window = SW_SHOWNORMAL,
                .wait_for_exit = false,
            };
            for (const auto& path : normalized_paths)
            {
                options.arguments.push_back(path.wstring());
            }
            return {.error = file_plan_error::none, .options = std::move(options)};
        }
        catch (...)
        {
            return {.error = file_plan_error::invalid_selection};
        }
    }

    directory_launch_plan create_directory_launch_plan(
        const installation& git_installation,
        const runner_command command,
        const std::filesystem::path& selected_path) noexcept
    {
        try
        {
            const auto script = directory_script(command);
            if (script.empty())
            {
                return {.error = directory_plan_error::unsupported_command};
            }

            const auto context = inspect_repository_context(selected_path);
            if (!context.selection_exists || context.selection_is_file || context.repository_root)
            {
                return {.error = directory_plan_error::invalid_selection};
            }
            if (!bash_is_available(git_installation))
            {
                return {.error = directory_plan_error::executable_unavailable};
            }

            return {
                .error = directory_plan_error::none,
                .options = process_launch_options{
                    .executable = git_installation.bash_executable,
                    .arguments = {
                        L"--login",
                        L"-c",
                        std::wstring(script),
                        L"menu11",
                    },
                    .working_directory = context.selected_path,
                    .creation_flags = CREATE_NEW_CONSOLE,
                    .show_window = SW_SHOWNORMAL,
                    .wait_for_exit = false,
                },
            };
        }
        catch (...)
        {
            return {.error = directory_plan_error::invalid_selection};
        }
    }
}

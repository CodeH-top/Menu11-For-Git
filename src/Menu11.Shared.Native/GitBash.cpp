#include "Menu11.Shared/GitBash.h"

#include "Menu11.Shared/RepositoryDetector.h"

#include <windows.h>

namespace menu11::git
{
    bash_launch_plan create_bash_launch_plan(
        const installation& git_installation,
        const std::filesystem::path& selected_path) noexcept
    {
        try
        {
            if (!git_installation.has_git_bash() ||
                GetFileAttributesW(git_installation.git_bash_executable.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                return {.error = bash_plan_error::executable_unavailable};
            }

            const auto context = inspect_repository_context(selected_path);
            if (!context.selection_exists || context.working_directory.empty())
            {
                return {.error = bash_plan_error::invalid_selection};
            }

            process_launch_options options{
                .executable = git_installation.git_bash_executable,
                .arguments = {L"--cd=" + context.working_directory.wstring()},
                .working_directory = context.working_directory,
                .show_window = SW_SHOWNORMAL,
                .wait_for_exit = false,
            };
            return {
                .error = bash_plan_error::none,
                .options = std::move(options),
            };
        }
        catch (...)
        {
            return {.error = bash_plan_error::invalid_selection};
        }
    }
}

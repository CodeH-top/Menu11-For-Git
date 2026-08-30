#include "Menu11.Shared/GitGui.h"

#include "Menu11.Shared/RepositoryDetector.h"

#include <windows.h>

namespace menu11::git
{
    gui_launch_plan create_gui_launch_plan(
        const installation& git_installation,
        const std::filesystem::path& selected_path) noexcept
    {
        try
        {
            if (!git_installation.has_gui() ||
                GetFileAttributesW(git_installation.gui_executable.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                return {.error = gui_plan_error::executable_unavailable};
            }

            const auto context = inspect_repository_context(selected_path);
            if (!context.selection_exists || context.working_directory.empty())
            {
                return {.error = gui_plan_error::invalid_selection};
            }

            process_launch_options options{
                .executable = git_installation.gui_executable,
                .arguments = {},
                .working_directory = context.working_directory,
                .show_window = SW_SHOWNORMAL,
                .wait_for_exit = false,
            };
            return {
                .error = gui_plan_error::none,
                .options = std::move(options),
            };
        }
        catch (...)
        {
            return {.error = gui_plan_error::invalid_selection};
        }
    }
}

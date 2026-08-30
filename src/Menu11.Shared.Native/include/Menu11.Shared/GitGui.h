#pragma once

#include "GitDetection.h"
#include "ProcessLauncher.h"

#include <filesystem>
#include <optional>

namespace menu11::git
{
    enum class gui_plan_error
    {
        none,
        executable_unavailable,
        invalid_selection,
    };

    struct gui_launch_plan
    {
        gui_plan_error error = gui_plan_error::none;
        std::optional<process_launch_options> options;

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return options.has_value();
        }
    };

    [[nodiscard]] gui_launch_plan create_gui_launch_plan(
        const installation& git_installation,
        const std::filesystem::path& selected_path) noexcept;
}

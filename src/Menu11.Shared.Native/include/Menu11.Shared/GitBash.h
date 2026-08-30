#pragma once

#include "GitDetection.h"
#include "ProcessLauncher.h"

#include <filesystem>
#include <optional>

namespace menu11::git
{
    enum class bash_plan_error
    {
        none,
        executable_unavailable,
        invalid_selection,
    };

    struct bash_launch_plan
    {
        bash_plan_error error = bash_plan_error::none;
        std::optional<process_launch_options> options;

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return options.has_value();
        }
    };

    [[nodiscard]] bash_launch_plan create_bash_launch_plan(
        const installation& git_installation,
        const std::filesystem::path& selected_path) noexcept;
}

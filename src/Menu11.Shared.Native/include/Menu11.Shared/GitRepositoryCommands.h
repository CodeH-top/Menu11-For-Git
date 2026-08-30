#pragma once

#include "GitDetection.h"
#include "ProcessLauncher.h"
#include "RunnerProtocol.h"

#include <filesystem>
#include <optional>
#include <span>

namespace menu11::git
{
    enum class repository_plan_error
    {
        none,
        executable_unavailable,
        invalid_selection,
        unsupported_command,
    };

    struct repository_launch_plan
    {
        repository_plan_error error = repository_plan_error::none;
        std::optional<process_launch_options> options;

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return options.has_value();
        }
    };

    [[nodiscard]] repository_launch_plan create_repository_launch_plan(
        const installation& git_installation,
        runner_command command,
        const std::filesystem::path& selected_path) noexcept;

    enum class file_plan_error
    {
        none,
        executable_unavailable,
        invalid_selection,
        unsupported_command,
    };

    struct file_launch_plan
    {
        file_plan_error error = file_plan_error::none;
        std::optional<process_launch_options> options;

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return options.has_value();
        }
    };

    [[nodiscard]] file_launch_plan create_file_launch_plan(
        const installation& git_installation,
        runner_command command,
        std::span<const std::filesystem::path> selected_paths) noexcept;

    enum class directory_plan_error
    {
        none,
        executable_unavailable,
        invalid_selection,
        unsupported_command,
    };

    struct directory_launch_plan
    {
        directory_plan_error error = directory_plan_error::none;
        std::optional<process_launch_options> options;

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return options.has_value();
        }
    };

    [[nodiscard]] directory_launch_plan create_directory_launch_plan(
        const installation& git_installation,
        runner_command command,
        const std::filesystem::path& selected_path) noexcept;
}

#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace menu11
{
    enum class runner_command
    {
        git_bash,
        git_gui,
        status,
        pull,
        fetch,
        push,
        commit,
        repository_log,
        branch,
        stash,
        file_add,
        file_diff,
        file_log,
        file_blame,
        file_restore,
        clone,
        init,
        settings,
    };

    enum class runner_parse_error
    {
        none,
        missing_command,
        duplicate_command,
        missing_option_value,
        unknown_command,
        unknown_option,
        missing_selection,
        empty_selection,
        too_many_selections,
    };

    struct runner_request
    {
        runner_command command;
        std::vector<std::filesystem::path> selections;
    };

    struct runner_parse_result
    {
        runner_parse_error error = runner_parse_error::none;
        std::optional<runner_request> request;

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return request.has_value();
        }
    };

    [[nodiscard]] runner_parse_result parse_runner_arguments(
        std::span<const std::wstring_view> arguments) noexcept;

    [[nodiscard]] std::wstring_view runner_command_token(runner_command command) noexcept;
}

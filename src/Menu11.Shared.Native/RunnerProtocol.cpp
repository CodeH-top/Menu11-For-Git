#include "Menu11.Shared/RunnerProtocol.h"

#include <array>

namespace menu11
{
    namespace
    {
        constexpr std::size_t maximum_selection_count = 512;

        struct command_mapping
        {
            std::wstring_view token;
            runner_command command;
        };

        constexpr std::array command_mappings{
            command_mapping{L"bash", runner_command::git_bash},
            command_mapping{L"gui", runner_command::git_gui},
            command_mapping{L"status", runner_command::status},
            command_mapping{L"pull", runner_command::pull},
            command_mapping{L"fetch", runner_command::fetch},
            command_mapping{L"push", runner_command::push},
            command_mapping{L"commit", runner_command::commit},
            command_mapping{L"repository-log", runner_command::repository_log},
            command_mapping{L"branch", runner_command::branch},
            command_mapping{L"stash", runner_command::stash},
            command_mapping{L"add", runner_command::file_add},
            command_mapping{L"diff", runner_command::file_diff},
            command_mapping{L"file-log", runner_command::file_log},
            command_mapping{L"blame", runner_command::file_blame},
            command_mapping{L"restore", runner_command::file_restore},
            command_mapping{L"clone", runner_command::clone},
            command_mapping{L"init", runner_command::init},
            command_mapping{L"settings", runner_command::settings},
        };

        [[nodiscard]] std::optional<runner_command> parse_command(std::wstring_view token) noexcept
        {
            for (const auto& mapping : command_mappings)
            {
                if (token == mapping.token)
                {
                    return mapping.command;
                }
            }
            return std::nullopt;
        }
    }

    runner_parse_result parse_runner_arguments(
        const std::span<const std::wstring_view> arguments) noexcept
    {
        try
        {
            std::optional<runner_command> command;
            std::vector<std::filesystem::path> selections;

            for (std::size_t index = 0; index < arguments.size(); ++index)
            {
                const auto option = arguments[index];
                if (option == L"--command")
                {
                    if (command)
                    {
                        return {.error = runner_parse_error::duplicate_command};
                    }
                    if (++index >= arguments.size())
                    {
                        return {.error = runner_parse_error::missing_option_value};
                    }

                    command = parse_command(arguments[index]);
                    if (!command)
                    {
                        return {.error = runner_parse_error::unknown_command};
                    }
                }
                else if (option == L"--path")
                {
                    if (++index >= arguments.size())
                    {
                        return {.error = runner_parse_error::missing_option_value};
                    }
                    if (arguments[index].empty())
                    {
                        return {.error = runner_parse_error::empty_selection};
                    }
                    if (selections.size() >= maximum_selection_count)
                    {
                        return {.error = runner_parse_error::too_many_selections};
                    }
                    selections.emplace_back(arguments[index]);
                }
                else
                {
                    return {.error = runner_parse_error::unknown_option};
                }
            }

            if (!command)
            {
                return {.error = runner_parse_error::missing_command};
            }
            if (*command != runner_command::settings && selections.empty())
            {
                return {.error = runner_parse_error::missing_selection};
            }

            return {
                .error = runner_parse_error::none,
                .request = runner_request{.command = *command, .selections = std::move(selections)},
            };
        }
        catch (...)
        {
            return {.error = runner_parse_error::unknown_option};
        }
    }

    std::wstring_view runner_command_token(const runner_command command) noexcept
    {
        for (const auto& mapping : command_mappings)
        {
            if (mapping.command == command)
            {
                return mapping.token;
            }
        }
        return {};
    }
}

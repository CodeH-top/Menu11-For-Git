#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>

namespace menu11
{
    enum class display_language
    {
        system,
        english,
        simplified_chinese,
    };

    enum class git_command : std::uint32_t
    {
        status = 1u << 0,
        pull = 1u << 1,
        fetch = 1u << 2,
        push = 1u << 3,
        commit = 1u << 4,
        repository_log = 1u << 5,
        branch = 1u << 6,
        stash = 1u << 7,
        file_add = 1u << 8,
        file_diff = 1u << 9,
        file_log = 1u << 10,
        file_blame = 1u << 11,
        file_restore = 1u << 12,
        clone = 1u << 13,
        init = 1u << 14,
    };

    inline constexpr std::uint32_t all_git_commands_mask = 0x00007FFFu;
    inline constexpr std::uint32_t default_git_commands_mask =
        static_cast<std::uint32_t>(git_command::status) |
        static_cast<std::uint32_t>(git_command::pull) |
        static_cast<std::uint32_t>(git_command::fetch) |
        static_cast<std::uint32_t>(git_command::push) |
        static_cast<std::uint32_t>(git_command::commit) |
        static_cast<std::uint32_t>(git_command::file_add) |
        static_cast<std::uint32_t>(git_command::file_diff) |
        static_cast<std::uint32_t>(git_command::clone) |
        static_cast<std::uint32_t>(git_command::init);

    struct settings
    {
        bool enabled = true;
        bool show_git_bash = true;
        bool show_git_gui = true;
        std::uint32_t enabled_commands = default_git_commands_mask;
        bool show_settings_command = true;
        bool show_icons = true;
        std::optional<std::filesystem::path> git_path;
        display_language language = display_language::system;

        [[nodiscard]] bool is_command_enabled(git_command command) const noexcept
        {
            return (enabled_commands & static_cast<std::uint32_t>(command)) != 0;
        }

        void set_command_enabled(git_command command, bool command_enabled) noexcept
        {
            const auto flag = static_cast<std::uint32_t>(command);
            if (command_enabled)
            {
                enabled_commands |= flag;
            }
            else
            {
                enabled_commands &= ~flag;
            }
            enabled_commands &= all_git_commands_mask;
        }
    };
}

#pragma once

#include <filesystem>
#include <optional>

namespace menu11
{
    struct repository_context
    {
        bool selection_exists = false;
        bool selection_is_file = false;
        std::filesystem::path selected_path;
        std::filesystem::path working_directory;
        std::optional<std::filesystem::path> repository_root;

        [[nodiscard]] bool is_repository() const noexcept
        {
            return repository_root.has_value();
        }
    };

    [[nodiscard]] std::optional<std::filesystem::path> find_repository_root(
        const std::filesystem::path& start_path) noexcept;

    [[nodiscard]] repository_context inspect_repository_context(
        const std::filesystem::path& selected_path) noexcept;
}

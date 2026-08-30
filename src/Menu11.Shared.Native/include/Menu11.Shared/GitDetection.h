#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace menu11::git
{
    enum class detection_source
    {
        machine_registry_64,
        machine_registry_32,
        user_registry_64,
        user_registry_32,
        path_environment,
        common_location,
        user_selected,
    };

    struct installation
    {
        std::filesystem::path install_root;
        std::filesystem::path git_executable;
        std::filesystem::path git_bash_executable;
        std::filesystem::path bash_executable;
        std::filesystem::path gui_executable;
        std::wstring version;
        detection_source source;

        [[nodiscard]] bool has_git_bash() const noexcept;
        [[nodiscard]] bool has_bash() const noexcept;
        [[nodiscard]] bool has_gui() const noexcept;
    };

    [[nodiscard]] std::optional<installation> inspect_candidate(
        const std::filesystem::path& candidate,
        detection_source source) noexcept;

    [[nodiscard]] std::optional<installation> detect_git_for_windows(
        const std::optional<std::filesystem::path>& user_selected_path = std::nullopt) noexcept;

    [[nodiscard]] std::wstring source_name(detection_source source);
}

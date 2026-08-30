#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace menu11
{
    [[nodiscard]] std::wstring quote_windows_argument(std::wstring_view argument);

    [[nodiscard]] std::wstring build_windows_command_line(
        const std::filesystem::path& executable,
        std::span<const std::wstring> arguments);
}

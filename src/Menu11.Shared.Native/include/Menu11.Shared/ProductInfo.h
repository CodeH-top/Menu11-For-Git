#pragma once

#include <string_view>

namespace menu11::product
{
    inline constexpr std::wstring_view display_name = L"Menu11 for Git";
    inline constexpr std::wstring_view internal_name = L"Menu11ForGit";
    inline constexpr std::wstring_view version = L"0.1.2";
    inline constexpr std::wstring_view registry_key = L"Software\\Menu11ForGit";
    inline constexpr std::wstring_view install_directory_name = L"Menu11ForGit";
    inline constexpr std::wstring_view app_executable = L"Menu11.exe";
    inline constexpr std::wstring_view runner_executable = L"Menu11.Runner.exe";
    inline constexpr std::wstring_view shell_library = L"Menu11.Shell.dll";
}

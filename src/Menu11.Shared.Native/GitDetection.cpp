#include "Menu11.Shared/GitDetection.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cwctype>
#include <system_error>
#include <vector>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Version.lib")

namespace menu11::git
{
    namespace
    {
        constexpr wchar_t git_registry_key[] = L"Software\\GitForWindows";
        constexpr wchar_t install_path_value[] = L"InstallPath";

        class registry_key final
        {
        public:
            registry_key() = default;
            ~registry_key()
            {
                if (handle_ != nullptr)
                {
                    RegCloseKey(handle_);
                }
            }

            registry_key(const registry_key&) = delete;
            registry_key& operator=(const registry_key&) = delete;

            [[nodiscard]] HKEY* address() noexcept
            {
                return &handle_;
            }

            [[nodiscard]] HKEY get() const noexcept
            {
                return handle_;
            }

        private:
            HKEY handle_ = nullptr;
        };

        [[nodiscard]] bool path_name_equals(const std::filesystem::path& path, const wchar_t* expected) noexcept
        {
            return _wcsicmp(path.filename().c_str(), expected) == 0;
        }

        [[nodiscard]] bool regular_file_exists(const std::filesystem::path& path) noexcept
        {
            std::error_code error;
            return std::filesystem::is_regular_file(path, error) && !error;
        }

        [[nodiscard]] bool directory_exists(const std::filesystem::path& path) noexcept
        {
            std::error_code error;
            return std::filesystem::is_directory(path, error) && !error;
        }

        [[nodiscard]] std::filesystem::path normalized_path(const std::filesystem::path& path) noexcept
        {
            std::error_code error;
            auto absolute = std::filesystem::absolute(path, error);
            if (error)
            {
                return path.lexically_normal();
            }

            auto canonical = std::filesystem::weakly_canonical(absolute, error);
            return error ? absolute.lexically_normal() : canonical;
        }

        [[nodiscard]] std::filesystem::path first_existing_file(
            const std::initializer_list<std::filesystem::path>& candidates) noexcept
        {
            for (const auto& candidate : candidates)
            {
                if (!candidate.empty() && regular_file_exists(candidate))
                {
                    return normalized_path(candidate);
                }
            }

            return {};
        }

        [[nodiscard]] std::wstring trim_registry_value(std::wstring value)
        {
            const auto is_space = [](const wchar_t character) { return std::iswspace(character) != 0; };
            value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), is_space));
            value.erase(std::find_if_not(value.rbegin(), value.rend(), is_space).base(), value.end());

            if (value.size() >= 2 && value.front() == L'"' && value.back() == L'"')
            {
                value = value.substr(1, value.size() - 2);
            }

            return value;
        }

        [[nodiscard]] std::optional<std::wstring> read_install_path(HKEY hive, REGSAM registry_view) noexcept
        {
            registry_key key;
            if (RegOpenKeyExW(
                    hive,
                    git_registry_key,
                    0,
                    KEY_QUERY_VALUE | registry_view,
                    key.address()) != ERROR_SUCCESS)
            {
                return std::nullopt;
            }

            DWORD type = 0;
            DWORD size = 0;
            if (RegQueryValueExW(key.get(), install_path_value, nullptr, &type, nullptr, &size) != ERROR_SUCCESS ||
                (type != REG_SZ && type != REG_EXPAND_SZ) || size < sizeof(wchar_t))
            {
                return std::nullopt;
            }

            std::vector<wchar_t> buffer((size / sizeof(wchar_t)) + 1, L'\0');
            if (RegQueryValueExW(
                    key.get(),
                    install_path_value,
                    nullptr,
                    &type,
                    reinterpret_cast<BYTE*>(buffer.data()),
                    &size) != ERROR_SUCCESS)
            {
                return std::nullopt;
            }

            std::wstring value(buffer.data());
            if (type == REG_EXPAND_SZ)
            {
                const DWORD required = ExpandEnvironmentStringsW(value.c_str(), nullptr, 0);
                if (required == 0)
                {
                    return std::nullopt;
                }

                std::vector<wchar_t> expanded(required, L'\0');
                if (ExpandEnvironmentStringsW(value.c_str(), expanded.data(), required) == 0)
                {
                    return std::nullopt;
                }
                value.assign(expanded.data());
            }

            value = trim_registry_value(std::move(value));
            return value.empty() ? std::nullopt : std::optional<std::wstring>(std::move(value));
        }

        [[nodiscard]] std::optional<std::filesystem::path> search_path_for_git() noexcept
        {
            const DWORD required = SearchPathW(nullptr, L"git.exe", nullptr, 0, nullptr, nullptr);
            if (required == 0)
            {
                return std::nullopt;
            }

            std::vector<wchar_t> buffer(static_cast<std::size_t>(required) + 1, L'\0');
            const DWORD written = SearchPathW(
                nullptr,
                L"git.exe",
                nullptr,
                static_cast<DWORD>(buffer.size()),
                buffer.data(),
                nullptr);
            if (written == 0 || written >= buffer.size())
            {
                return std::nullopt;
            }

            return std::filesystem::path(buffer.data());
        }

        [[nodiscard]] std::optional<std::filesystem::path> environment_path(const wchar_t* variable) noexcept
        {
            const DWORD required = GetEnvironmentVariableW(variable, nullptr, 0);
            if (required == 0)
            {
                return std::nullopt;
            }

            std::vector<wchar_t> buffer(required, L'\0');
            const DWORD written = GetEnvironmentVariableW(variable, buffer.data(), required);
            if (written == 0 || written >= required)
            {
                return std::nullopt;
            }

            return std::filesystem::path(buffer.data());
        }

        [[nodiscard]] std::wstring executable_version(const std::filesystem::path& executable) noexcept
        {
            DWORD ignored = 0;
            const DWORD size = GetFileVersionInfoSizeW(executable.c_str(), &ignored);
            if (size == 0)
            {
                return {};
            }

            std::vector<std::byte> data(size);
            if (!GetFileVersionInfoW(executable.c_str(), 0, size, data.data()))
            {
                return {};
            }

            struct language_and_code_page
            {
                WORD language;
                WORD code_page;
            };

            language_and_code_page* translations = nullptr;
            UINT translations_size = 0;
            if (VerQueryValueW(
                    data.data(),
                    L"\\VarFileInfo\\Translation",
                    reinterpret_cast<void**>(&translations),
                    &translations_size) &&
                translations != nullptr)
            {
                const auto count = translations_size / sizeof(language_and_code_page);
                for (UINT index = 0; index < count; ++index)
                {
                    wchar_t query[64]{};
                    swprintf_s(
                        query,
                        L"\\StringFileInfo\\%04x%04x\\ProductVersion",
                        translations[index].language,
                        translations[index].code_page);

                    wchar_t* value = nullptr;
                    UINT value_size = 0;
                    if (VerQueryValueW(
                            data.data(),
                            query,
                            reinterpret_cast<void**>(&value),
                            &value_size) &&
                        value != nullptr && value_size > 1)
                    {
                        return std::wstring(value, value_size - 1);
                    }
                }
            }

            VS_FIXEDFILEINFO* fixed_info = nullptr;
            UINT fixed_info_size = 0;
            if (!VerQueryValueW(
                    data.data(),
                    L"\\",
                    reinterpret_cast<void**>(&fixed_info),
                    &fixed_info_size) ||
                fixed_info == nullptr || fixed_info_size < sizeof(VS_FIXEDFILEINFO))
            {
                return {};
            }

            return std::to_wstring(HIWORD(fixed_info->dwFileVersionMS)) + L"." +
                std::to_wstring(LOWORD(fixed_info->dwFileVersionMS)) + L"." +
                std::to_wstring(HIWORD(fixed_info->dwFileVersionLS)) + L"." +
                std::to_wstring(LOWORD(fixed_info->dwFileVersionLS));
        }
    }

    bool installation::has_git_bash() const noexcept
    {
        return !git_bash_executable.empty();
    }

    bool installation::has_bash() const noexcept
    {
        return !bash_executable.empty();
    }

    bool installation::has_gui() const noexcept
    {
        return !gui_executable.empty();
    }

    std::optional<installation> inspect_candidate(
        const std::filesystem::path& candidate,
        const detection_source source) noexcept
    {
        try
        {
            if (candidate.empty())
            {
                return std::nullopt;
            }

            auto normalized_candidate = normalized_path(candidate);
            std::filesystem::path install_root;
            std::filesystem::path explicit_git;

            if (regular_file_exists(normalized_candidate))
            {
                if (!path_name_equals(normalized_candidate, L"git.exe"))
                {
                    return std::nullopt;
                }

                explicit_git = normalized_candidate;
                install_root = normalized_candidate.parent_path();
                if (path_name_equals(install_root, L"cmd") || path_name_equals(install_root, L"bin"))
                {
                    install_root = install_root.parent_path();
                }
            }
            else if (directory_exists(normalized_candidate))
            {
                install_root = normalized_candidate;
                if (path_name_equals(install_root, L"cmd") || path_name_equals(install_root, L"bin"))
                {
                    explicit_git = install_root / L"git.exe";
                    install_root = install_root.parent_path();
                }
            }
            else
            {
                return std::nullopt;
            }

            install_root = normalized_path(install_root);
            const auto git_executable = first_existing_file({
                explicit_git,
                install_root / L"cmd" / L"git.exe",
                install_root / L"bin" / L"git.exe",
                install_root / L"git.exe",
            });
            if (git_executable.empty())
            {
                return std::nullopt;
            }

            installation result{
                .install_root = install_root,
                .git_executable = git_executable,
                .git_bash_executable = first_existing_file({install_root / L"git-bash.exe"}),
                .bash_executable = first_existing_file({install_root / L"bin" / L"bash.exe"}),
                .gui_executable = first_existing_file({
                    install_root / L"cmd" / L"git-gui.exe",
                    install_root / L"bin" / L"git-gui.exe",
                    install_root / L"git-gui.exe",
                }),
                .version = executable_version(git_executable),
                .source = source,
            };
            return result;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::optional<installation> detect_git_for_windows(
        const std::optional<std::filesystem::path>& user_selected_path) noexcept
    {
        try
        {
            struct registry_location
            {
                HKEY hive;
                REGSAM view;
                detection_source source;
            };

            const std::array registry_locations{
                registry_location{HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY, detection_source::machine_registry_64},
                registry_location{HKEY_LOCAL_MACHINE, KEY_WOW64_32KEY, detection_source::machine_registry_32},
                registry_location{HKEY_CURRENT_USER, KEY_WOW64_64KEY, detection_source::user_registry_64},
                registry_location{HKEY_CURRENT_USER, KEY_WOW64_32KEY, detection_source::user_registry_32},
            };

            for (const auto& location : registry_locations)
            {
                if (const auto registry_path = read_install_path(location.hive, location.view))
                {
                    if (auto installation = inspect_candidate(*registry_path, location.source))
                    {
                        return installation;
                    }
                }
            }

            if (const auto path_git = search_path_for_git())
            {
                if (auto installation = inspect_candidate(*path_git, detection_source::path_environment))
                {
                    return installation;
                }
            }

            for (const auto* variable : {L"ProgramW6432", L"ProgramFiles", L"ProgramFiles(x86)"})
            {
                if (const auto base = environment_path(variable))
                {
                    if (auto installation = inspect_candidate(*base / L"Git", detection_source::common_location))
                    {
                        return installation;
                    }
                }
            }

            if (const auto local_app_data = environment_path(L"LOCALAPPDATA"))
            {
                if (auto installation = inspect_candidate(
                        *local_app_data / L"Programs" / L"Git",
                        detection_source::common_location))
                {
                    return installation;
                }
            }

            if (user_selected_path)
            {
                return inspect_candidate(*user_selected_path, detection_source::user_selected);
            }

            return std::nullopt;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::wstring source_name(const detection_source source)
    {
        switch (source)
        {
        case detection_source::machine_registry_64:
            return L"HKLM registry (64-bit)";
        case detection_source::machine_registry_32:
            return L"HKLM registry (32-bit)";
        case detection_source::user_registry_64:
            return L"HKCU registry (64-bit)";
        case detection_source::user_registry_32:
            return L"HKCU registry (32-bit)";
        case detection_source::path_environment:
            return L"PATH";
        case detection_source::common_location:
            return L"common installation location";
        case detection_source::user_selected:
            return L"user-selected path";
        }

        return L"unknown";
    }
}

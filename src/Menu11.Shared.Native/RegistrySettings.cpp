#include "Menu11.Shared/RegistrySettings.h"

#include "Menu11.Shared/ProductInfo.h"

#include <vector>

#pragma comment(lib, "Advapi32.lib")

namespace menu11
{
    namespace
    {
        constexpr wchar_t enabled_name[] = L"Enabled";
        constexpr wchar_t git_path_name[] = L"GitPath";
        constexpr wchar_t show_git_bash_name[] = L"ShowGitBash";
        constexpr wchar_t show_git_gui_name[] = L"ShowGitGUI";
        constexpr wchar_t enabled_commands_name[] = L"EnabledCommands";
        constexpr wchar_t show_settings_command_name[] = L"ShowSettingsCommand";
        constexpr wchar_t show_icons_name[] = L"ShowIcons";
        constexpr wchar_t language_name[] = L"Language";

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

        void read_boolean(HKEY key, const wchar_t* name, bool& destination) noexcept
        {
            DWORD type = 0;
            DWORD value = 0;
            DWORD size = sizeof(value);
            if (RegQueryValueExW(
                    key,
                    name,
                    nullptr,
                    &type,
                    reinterpret_cast<BYTE*>(&value),
                    &size) == ERROR_SUCCESS &&
                type == REG_DWORD && size == sizeof(value) && value <= 1)
            {
                destination = value != 0;
            }
        }

        void read_commands(HKEY key, std::uint32_t& destination) noexcept
        {
            DWORD type = 0;
            DWORD value = 0;
            DWORD size = sizeof(value);
            if (RegQueryValueExW(
                    key,
                    enabled_commands_name,
                    nullptr,
                    &type,
                    reinterpret_cast<BYTE*>(&value),
                    &size) == ERROR_SUCCESS &&
                type == REG_DWORD && size == sizeof(value))
            {
                destination = value & all_git_commands_mask;
            }
        }

        void read_git_path(HKEY key, std::optional<std::filesystem::path>& destination) noexcept
        {
            DWORD type = 0;
            DWORD size = 0;
            if (RegQueryValueExW(key, git_path_name, nullptr, &type, nullptr, &size) != ERROR_SUCCESS ||
                (type != REG_SZ && type != REG_EXPAND_SZ) || size < sizeof(wchar_t))
            {
                return;
            }

            std::vector<wchar_t> buffer((size / sizeof(wchar_t)) + 1, L'\0');
            if (RegQueryValueExW(
                    key,
                    git_path_name,
                    nullptr,
                    &type,
                    reinterpret_cast<BYTE*>(buffer.data()),
                    &size) != ERROR_SUCCESS)
            {
                return;
            }

            std::wstring path(buffer.data());
            if (type == REG_EXPAND_SZ)
            {
                const DWORD required = ExpandEnvironmentStringsW(path.c_str(), nullptr, 0);
                if (required == 0)
                {
                    return;
                }

                std::vector<wchar_t> expanded(required, L'\0');
                if (ExpandEnvironmentStringsW(path.c_str(), expanded.data(), required) == 0)
                {
                    return;
                }
                path.assign(expanded.data());
            }

            if (!path.empty())
            {
                destination = std::filesystem::path(std::move(path));
            }
        }

        void read_language(HKEY key, display_language& destination) noexcept
        {
            wchar_t buffer[16]{};
            DWORD type = 0;
            DWORD size = sizeof(buffer);
            if (RegQueryValueExW(
                    key,
                    language_name,
                    nullptr,
                    &type,
                    reinterpret_cast<BYTE*>(buffer),
                    &size) != ERROR_SUCCESS ||
                type != REG_SZ)
            {
                return;
            }

            if (CompareStringOrdinal(buffer, -1, L"en-US", -1, TRUE) == CSTR_EQUAL)
            {
                destination = display_language::english;
            }
            else if (CompareStringOrdinal(buffer, -1, L"zh-CN", -1, TRUE) == CSTR_EQUAL)
            {
                destination = display_language::simplified_chinese;
            }
        }

        [[nodiscard]] LSTATUS write_dword(HKEY key, const wchar_t* name, DWORD value) noexcept
        {
            return RegSetValueExW(
                key,
                name,
                0,
                REG_DWORD,
                reinterpret_cast<const BYTE*>(&value),
                sizeof(value));
        }
    }

    registry_settings_store::registry_settings_store()
        : registry_settings_store(HKEY_CURRENT_USER, std::wstring(product::registry_key))
    {
    }

    registry_settings_store::registry_settings_store(HKEY root, std::wstring subkey)
        : root_(root), subkey_(std::move(subkey))
    {
    }

    settings registry_settings_store::load() const noexcept
    {
        settings value;
        registry_key key;
        const LSTATUS status = RegOpenKeyExW(root_, subkey_.c_str(), 0, KEY_QUERY_VALUE, key.address());
        if (status != ERROR_SUCCESS)
        {
            return value;
        }

        read_boolean(key.get(), enabled_name, value.enabled);
        read_boolean(key.get(), show_git_bash_name, value.show_git_bash);
        read_boolean(key.get(), show_git_gui_name, value.show_git_gui);
        read_commands(key.get(), value.enabled_commands);
        read_boolean(key.get(), show_settings_command_name, value.show_settings_command);
        read_boolean(key.get(), show_icons_name, value.show_icons);
        read_git_path(key.get(), value.git_path);
        read_language(key.get(), value.language);
        return value;
    }

    LSTATUS registry_settings_store::save(const settings& value) const noexcept
    {
        registry_key key;
        LSTATUS status = RegCreateKeyExW(
            root_,
            subkey_.c_str(),
            0,
            nullptr,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            nullptr,
            key.address(),
            nullptr);
        if (status != ERROR_SUCCESS)
        {
            return status;
        }

        const struct dword_value
        {
            const wchar_t* name;
            DWORD value;
        } values[] = {
            {enabled_name, value.enabled ? 1u : 0u},
            {show_git_bash_name, value.show_git_bash ? 1u : 0u},
            {show_git_gui_name, value.show_git_gui ? 1u : 0u},
            {enabled_commands_name, value.enabled_commands & all_git_commands_mask},
            {show_settings_command_name, value.show_settings_command ? 1u : 0u},
            {show_icons_name, value.show_icons ? 1u : 0u},
        };

        for (const auto& entry : values)
        {
            status = write_dword(key.get(), entry.name, entry.value);
            if (status != ERROR_SUCCESS)
            {
                return status;
            }
        }

        const wchar_t* language = L"system";
        if (value.language == display_language::english)
        {
            language = L"en-US";
        }
        else if (value.language == display_language::simplified_chinese)
        {
            language = L"zh-CN";
        }
        status = RegSetValueExW(
            key.get(),
            language_name,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(language),
            static_cast<DWORD>((wcslen(language) + 1) * sizeof(wchar_t)));
        if (status != ERROR_SUCCESS)
        {
            return status;
        }

        if (!value.git_path || value.git_path->empty())
        {
            status = RegDeleteValueW(key.get(), git_path_name);
            return status == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : status;
        }

        const auto path = value.git_path->wstring();
        return RegSetValueExW(
            key.get(),
            git_path_name,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(path.c_str()),
            static_cast<DWORD>((path.size() + 1) * sizeof(wchar_t)));
    }

    LSTATUS registry_settings_store::remove() const noexcept
    {
        const LSTATUS status = RegDeleteTreeW(root_, subkey_.c_str());
        return status == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : status;
    }
}

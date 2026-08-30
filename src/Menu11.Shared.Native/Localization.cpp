#include "Menu11.Shared/Localization.h"

#include <windows.h>

#include <array>

namespace menu11
{
    namespace
    {
        struct localized_pair
        {
            std::wstring_view english;
            std::wstring_view simplified_chinese;
        };

        constexpr std::array strings{
            localized_pair{L"Git Bash Here", L"在此处打开 Git Bash"},
            localized_pair{L"Git GUI Here", L"在此处打开 Git GUI"},
            localized_pair{L"Status", L"Status"},
            localized_pair{L"Pull", L"Pull"},
            localized_pair{L"Fetch", L"Fetch"},
            localized_pair{L"Push", L"Push"},
            localized_pair{L"Commit", L"Commit"},
            localized_pair{L"Log", L"Log"},
            localized_pair{L"Branch", L"Branch"},
            localized_pair{L"Stash", L"Stash"},
            localized_pair{L"Add", L"Add"},
            localized_pair{L"Diff", L"Diff"},
            localized_pair{L"Blame", L"Blame"},
            localized_pair{L"Restore", L"Restore"},
            localized_pair{L"Clone...", L"Clone..."},
            localized_pair{L"Init", L"Init"},
            localized_pair{L"Settings", L"设置"},
        };

        static_assert(strings.size() == static_cast<std::size_t>(localized_string::count));
    }

    display_language resolve_display_language(const display_language language) noexcept
    {
        if (language != display_language::system)
        {
            return language;
        }

        return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_CHINESE
            ? display_language::simplified_chinese
            : display_language::english;
    }

    std::wstring_view localize(
        const localized_string value,
        const display_language language) noexcept
    {
        const auto index = static_cast<std::size_t>(value);
        if (index >= strings.size())
        {
            return {};
        }

        const auto& text = strings[index];
        return resolve_display_language(language) == display_language::simplified_chinese
            ? text.simplified_chinese
            : text.english;
    }
}

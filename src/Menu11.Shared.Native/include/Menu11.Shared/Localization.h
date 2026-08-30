#pragma once

#include "Settings.h"

#include <string_view>

namespace menu11
{
    enum class localized_string
    {
        git_bash_here,
        git_gui_here,
        status,
        pull,
        fetch,
        push,
        commit,
        log,
        branch,
        stash,
        add,
        diff,
        blame,
        restore,
        clone,
        init_repository,
        settings,
        count,
    };

    [[nodiscard]] display_language resolve_display_language(display_language language) noexcept;
    [[nodiscard]] std::wstring_view localize(
        localized_string value,
        display_language language) noexcept;
}

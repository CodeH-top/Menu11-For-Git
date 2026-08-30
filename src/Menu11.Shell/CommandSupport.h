#pragma once

#include <windows.h>

#include <string_view>

namespace menu11::shell
{
    [[nodiscard]] HRESULT duplicate_string(std::wstring_view value, PWSTR* output) noexcept;
    [[nodiscard]] HRESULT get_shell_icon(unsigned int resource_id, PWSTR* icon) noexcept;
    [[nodiscard]] HRESULT get_no_tool_tip(PWSTR* tool_tip) noexcept;
}

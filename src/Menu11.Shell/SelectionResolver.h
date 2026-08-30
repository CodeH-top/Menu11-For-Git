#pragma once

#include <shobjidl_core.h>

#include <filesystem>
#include <vector>

namespace menu11::shell
{
    [[nodiscard]] HRESULT resolve_selection(
        IShellItemArray* items,
        std::vector<std::filesystem::path>& paths) noexcept;
    [[nodiscard]] HRESULT resolve_selection_or_site(
        IShellItemArray* items,
        IUnknown* site,
        std::vector<std::filesystem::path>& paths) noexcept;
}

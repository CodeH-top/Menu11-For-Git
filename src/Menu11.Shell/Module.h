#pragma once

#include <windows.h>

namespace menu11::shell
{
    void set_module_instance(HMODULE module) noexcept;
    [[nodiscard]] HMODULE module_instance() noexcept;
    void add_module_reference() noexcept;
    void release_module_reference() noexcept;
    [[nodiscard]] long module_reference_count() noexcept;
}

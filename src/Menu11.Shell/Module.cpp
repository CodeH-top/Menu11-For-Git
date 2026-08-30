#include "Module.h"

#include <atomic>

namespace menu11::shell
{
    namespace
    {
        HMODULE current_module = nullptr;
        std::atomic_long reference_count = 0;
    }

    void set_module_instance(const HMODULE module) noexcept
    {
        current_module = module;
    }

    HMODULE module_instance() noexcept
    {
        return current_module;
    }

    void add_module_reference() noexcept
    {
        reference_count.fetch_add(1, std::memory_order_relaxed);
    }

    void release_module_reference() noexcept
    {
        reference_count.fetch_sub(1, std::memory_order_release);
    }

    long module_reference_count() noexcept
    {
        return reference_count.load(std::memory_order_acquire);
    }
}

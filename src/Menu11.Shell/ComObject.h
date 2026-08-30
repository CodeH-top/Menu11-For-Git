#pragma once

#include "Module.h"

#include <windows.h>

#include <atomic>

namespace menu11::shell
{
    class com_object
    {
    protected:
        com_object() noexcept
        {
            add_module_reference();
        }

        virtual ~com_object()
        {
            release_module_reference();
        }

        [[nodiscard]] ULONG add_reference() noexcept
        {
            return reference_count_.fetch_add(1, std::memory_order_relaxed) + 1;
        }

        [[nodiscard]] ULONG release_reference() noexcept
        {
            const ULONG remaining = reference_count_.fetch_sub(1, std::memory_order_acq_rel) - 1;
            if (remaining == 0)
            {
                delete this;
            }
            return remaining;
        }

    private:
        std::atomic_ulong reference_count_ = 1;
    };
}

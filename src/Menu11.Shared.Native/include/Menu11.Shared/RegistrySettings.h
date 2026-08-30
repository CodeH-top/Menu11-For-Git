#pragma once

#include "Settings.h"

#include <windows.h>

#include <string>

namespace menu11
{
    class registry_settings_store final
    {
    public:
        registry_settings_store();
        registry_settings_store(HKEY root, std::wstring subkey);

        [[nodiscard]] settings load() const noexcept;
        [[nodiscard]] LSTATUS save(const settings& value) const noexcept;
        [[nodiscard]] LSTATUS remove() const noexcept;

    private:
        HKEY root_;
        std::wstring subkey_;
    };
}

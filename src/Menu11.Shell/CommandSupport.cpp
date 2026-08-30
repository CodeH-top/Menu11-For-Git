#include "CommandSupport.h"

#include "Menu11.Shared/RegistrySettings.h"
#include "Module.h"

#include <cstring>
#include <string>
#include <vector>

namespace menu11::shell
{
    HRESULT duplicate_string(const std::wstring_view value, PWSTR* output) noexcept
    {
        if (output == nullptr)
        {
            return E_POINTER;
        }
        *output = nullptr;

        const auto bytes = (value.size() + 1) * sizeof(wchar_t);
        auto* buffer = static_cast<PWSTR>(CoTaskMemAlloc(bytes));
        if (buffer == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        std::memcpy(buffer, value.data(), value.size() * sizeof(wchar_t));
        buffer[value.size()] = L'\0';
        *output = buffer;
        return S_OK;
    }

    HRESULT get_shell_icon(const unsigned int resource_id, PWSTR* icon) noexcept
    {
        if (icon == nullptr)
        {
            return E_POINTER;
        }
        *icon = nullptr;

        const registry_settings_store settings_store;
        if (!settings_store.load().show_icons)
        {
            return E_NOTIMPL;
        }

        try
        {
            std::vector<wchar_t> buffer(512, L'\0');
            for (;;)
            {
                const DWORD length = GetModuleFileNameW(
                    module_instance(),
                    buffer.data(),
                    static_cast<DWORD>(buffer.size()));
                if (length == 0)
                {
                    return HRESULT_FROM_WIN32(GetLastError());
                }
                if (length < buffer.size() - 1)
                {
                    return duplicate_string(
                        std::wstring(buffer.data(), length) + L",-" + std::to_wstring(resource_id),
                        icon);
                }
                if (buffer.size() >= 32768)
                {
                    return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                }
                buffer.resize(buffer.size() * 2, L'\0');
            }
        }
        catch (...)
        {
            return E_OUTOFMEMORY;
        }
    }

    HRESULT get_no_tool_tip(PWSTR* tool_tip) noexcept
    {
        if (tool_tip == nullptr)
        {
            return E_POINTER;
        }
        *tool_tip = nullptr;
        return E_NOTIMPL;
    }
}

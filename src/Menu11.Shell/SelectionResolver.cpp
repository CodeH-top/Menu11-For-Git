#include "SelectionResolver.h"

#include <servprov.h>
#include <shlguid.h>
#include <shlobj_core.h>
#include <wrl/client.h>

#include <cwchar>

namespace
{
    struct cotaskmem_id_list final
    {
        PIDLIST_ABSOLUTE value = nullptr;

        cotaskmem_id_list() noexcept = default;

        ~cotaskmem_id_list()
        {
            CoTaskMemFree(value);
        }

        cotaskmem_id_list(const cotaskmem_id_list&) = delete;
        cotaskmem_id_list& operator=(const cotaskmem_id_list&) = delete;
    };
}

namespace menu11::shell
{
    HRESULT resolve_selection(
        IShellItemArray* items,
        std::vector<std::filesystem::path>& paths) noexcept
    {
        paths.clear();
        if (items == nullptr)
        {
            return E_INVALIDARG;
        }

        try
        {
            DWORD count = 0;
            HRESULT result = items->GetCount(&count);
            if (FAILED(result))
            {
                return result;
            }
            if (count == 0)
            {
                return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            }

            paths.reserve(count);
            for (DWORD index = 0; index < count; ++index)
            {
                Microsoft::WRL::ComPtr<IShellItem> item;
                result = items->GetItemAt(index, &item);
                if (FAILED(result))
                {
                    return result;
                }

                PWSTR display_name = nullptr;
                result = item->GetDisplayName(SIGDN_FILESYSPATH, &display_name);
                if (FAILED(result))
                {
                    return result;
                }

                std::filesystem::path path(display_name);
                CoTaskMemFree(display_name);
                if (path.empty())
                {
                    return E_INVALIDARG;
                }
                paths.push_back(std::move(path));
            }

            return S_OK;
        }
        catch (...)
        {
            paths.clear();
            return E_OUTOFMEMORY;
        }
    }

    HRESULT resolve_selection_or_site(
        IShellItemArray* items,
        IUnknown* site,
        std::vector<std::filesystem::path>& paths) noexcept
    {
        if (items != nullptr)
        {
            return resolve_selection(items, paths);
        }

        paths.clear();
        if (site == nullptr)
        {
            return E_INVALIDARG;
        }

        try
        {
            Microsoft::WRL::ComPtr<IServiceProvider> service_provider;
            HRESULT result = site->QueryInterface(IID_PPV_ARGS(&service_provider));
            if (FAILED(result))
            {
                return result;
            }

            Microsoft::WRL::ComPtr<IShellBrowser> browser;
            result = service_provider->QueryService(
                SID_STopLevelBrowser,
                IID_PPV_ARGS(&browser));
            if (FAILED(result))
            {
                return result;
            }

            Microsoft::WRL::ComPtr<IShellView> shell_view;
            result = browser->QueryActiveShellView(&shell_view);
            if (FAILED(result))
            {
                return result;
            }

            Microsoft::WRL::ComPtr<IFolderView> folder_view;
            result = shell_view.As(&folder_view);
            if (FAILED(result))
            {
                return result;
            }

            Microsoft::WRL::ComPtr<IShellItemArray> folder_items;
            result = folder_view->GetFolder(IID_PPV_ARGS(&folder_items));
            if (SUCCEEDED(result))
            {
                return resolve_selection(folder_items.Get(), paths);
            }

            Microsoft::WRL::ComPtr<IPersistFolder2> persisted_folder;
            result = folder_view->GetFolder(IID_PPV_ARGS(&persisted_folder));
            if (FAILED(result))
            {
                return result;
            }

            cotaskmem_id_list id_list;
            result = persisted_folder->GetCurFolder(&id_list.value);
            if (FAILED(result))
            {
                return result;
            }
            std::wstring path(32768, L'\0');
            if (!SHGetPathFromIDListEx(
                    id_list.value,
                    path.data(),
                    static_cast<DWORD>(path.size()),
                    GPFIDL_DEFAULT))
            {
                return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
            }
            path.resize(std::wcslen(path.c_str()));
            if (path.empty())
            {
                return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
            }
            paths.emplace_back(std::move(path));
            return S_OK;
        }
        catch (...)
        {
            paths.clear();
            return E_OUTOFMEMORY;
        }
    }
}

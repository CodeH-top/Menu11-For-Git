#include "GitMenuCommand.h"

#include "CommandSupport.h"
#include "GitChildCommand.h"
#include "GitCommandDefinition.h"
#include "GitCommandEnumerator.h"
#include "Menu11.Shared/RegistrySettings.h"
#include "Resource.h"
#include "ShellGuids.h"

#include <windows.h>

#include <new>
#include <vector>

namespace menu11::shell
{
    HRESULT git_menu_command::QueryInterface(REFIID interface_id, void** object)
    {
        if (object == nullptr)
        {
            return E_POINTER;
        }
        *object = nullptr;

        if (interface_id == IID_IUnknown || interface_id == __uuidof(IExplorerCommand))
        {
            *object = static_cast<IExplorerCommand*>(this);
            AddRef();
            return S_OK;
        }
        if (interface_id == __uuidof(IObjectWithSite))
        {
            *object = static_cast<IObjectWithSite*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG git_menu_command::AddRef()
    {
        return add_reference();
    }

    ULONG git_menu_command::Release()
    {
        return release_reference();
    }

    HRESULT git_menu_command::GetTitle(IShellItemArray*, PWSTR* title)
    {
        return duplicate_string(L"Git", title);
    }

    HRESULT git_menu_command::GetIcon(IShellItemArray*, PWSTR* icon)
    {
        return get_shell_icon(IDI_MENU11_GIT, icon);
    }

    HRESULT git_menu_command::GetToolTip(IShellItemArray*, PWSTR* tool_tip)
    {
        return get_no_tool_tip(tool_tip);
    }

    HRESULT git_menu_command::GetCanonicalName(GUID* canonical_name)
    {
        if (canonical_name == nullptr)
        {
            return E_POINTER;
        }
        *canonical_name = CLSID_Menu11GitMenuCommand;
        return S_OK;
    }

    HRESULT git_menu_command::GetState(IShellItemArray* items, BOOL, EXPCMDSTATE* state)
    {
        if (state == nullptr)
        {
            return E_POINTER;
        }

        const registry_settings_store settings_store;
        const auto current_settings = settings_store.load();
        bool has_visible_child = false;
        if (current_settings.enabled)
        {
            for (const auto& definition : git_command_definitions())
            {
                if (is_git_command_enabled(definition, current_settings) &&
                    is_git_command_visible(definition, items, site_.Get()))
                {
                    has_visible_child = true;
                    break;
                }
            }
        }
        *state = has_visible_child ? ECS_ENABLED : ECS_HIDDEN;
        return S_OK;
    }

    HRESULT git_menu_command::Invoke(IShellItemArray*, IBindCtx*)
    {
        return E_NOTIMPL;
    }

    HRESULT git_menu_command::GetFlags(EXPCMDFLAGS* flags)
    {
        if (flags == nullptr)
        {
            return E_POINTER;
        }
        *flags = ECF_HASSUBCOMMANDS;
        return S_OK;
    }

    HRESULT git_menu_command::EnumSubCommands(IEnumExplorerCommand** commands)
    {
        if (commands == nullptr)
        {
            return E_POINTER;
        }
        *commands = nullptr;

        try
        {
            const registry_settings_store settings_store;
            const auto current_settings = settings_store.load();
            std::vector<const git_command_definition*> enabled_definitions;
            if (current_settings.enabled)
            {
                for (const auto& definition : git_command_definitions())
                {
                    if (is_git_command_enabled(definition, current_settings))
                    {
                        enabled_definitions.push_back(&definition);
                    }
                }
            }
            return create_git_command_enumerator(
                std::move(enabled_definitions),
                site_.Get(),
                commands);
        }
        catch (...)
        {
            return E_OUTOFMEMORY;
        }
    }

    HRESULT git_menu_command::SetSite(IUnknown* site)
    {
        site_ = site;
        return S_OK;
    }

    HRESULT git_menu_command::GetSite(REFIID interface_id, void** site)
    {
        if (site == nullptr)
        {
            return E_POINTER;
        }
        *site = nullptr;
        return site_ == nullptr
            ? E_FAIL
            : site_->QueryInterface(interface_id, site);
    }

    HRESULT create_git_menu_command(REFIID interface_id, void** object) noexcept
    {
        if (object == nullptr)
        {
            return E_POINTER;
        }
        *object = nullptr;

        auto* command = new (std::nothrow) git_menu_command();
        if (command == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        const HRESULT result = command->QueryInterface(interface_id, object);
        command->Release();
        return result;
    }
}

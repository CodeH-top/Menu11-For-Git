#include "GeneralCommand.h"

#include "CommandSupport.h"
#include "Menu11.Shared/RegistrySettings.h"
#include "SelectionResolver.h"
#include "ShellGuids.h"
#include "ShellLauncher.h"
#include "Resource.h"

#include <windows.h>

#include <new>
#include <vector>

namespace menu11::shell
{
    namespace
    {
        constexpr general_command_definition git_bash_definition{
            .canonical_name = CLSID_Menu11GitBashCommand,
            .title = localized_string::git_bash_here,
            .command = runner_command::git_bash,
            .enabled_member = &settings::show_git_bash,
            .icon_resource_id = IDI_MENU11_GIT_BASH,
        };

        constexpr general_command_definition git_gui_definition{
            .canonical_name = CLSID_Menu11GitGuiCommand,
            .title = localized_string::git_gui_here,
            .command = runner_command::git_gui,
            .enabled_member = &settings::show_git_gui,
            .icon_resource_id = IDI_MENU11_GIT_GUI,
        };

        [[nodiscard]] HRESULT create_command(
            const general_command_definition& definition,
            REFIID interface_id,
            void** object) noexcept
        {
            if (object == nullptr)
            {
                return E_POINTER;
            }
            *object = nullptr;

            auto* command = new (std::nothrow) general_command(definition);
            if (command == nullptr)
            {
                return E_OUTOFMEMORY;
            }

            const HRESULT result = command->QueryInterface(interface_id, object);
            command->Release();
            return result;
        }
    }

    general_command::general_command(const general_command_definition& definition) noexcept
        : definition_(&definition)
    {
    }

    HRESULT general_command::QueryInterface(REFIID interface_id, void** object)
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

    ULONG general_command::AddRef()
    {
        return add_reference();
    }

    ULONG general_command::Release()
    {
        return release_reference();
    }

    HRESULT general_command::GetTitle(IShellItemArray*, PWSTR* title)
    {
        const auto current_settings = registry_settings_store().load();
        return duplicate_string(localize(definition_->title, current_settings.language), title);
    }

    HRESULT general_command::GetIcon(IShellItemArray*, PWSTR* icon)
    {
        return get_shell_icon(definition_->icon_resource_id, icon);
    }

    HRESULT general_command::GetToolTip(IShellItemArray*, PWSTR* tool_tip)
    {
        return get_no_tool_tip(tool_tip);
    }

    HRESULT general_command::GetCanonicalName(GUID* canonical_name)
    {
        if (canonical_name == nullptr)
        {
            return E_POINTER;
        }
        *canonical_name = definition_->canonical_name;
        return S_OK;
    }

    HRESULT general_command::GetState(IShellItemArray*, BOOL, EXPCMDSTATE* state)
    {
        if (state == nullptr)
        {
            return E_POINTER;
        }

        const registry_settings_store settings_store;
        const auto current_settings = settings_store.load();
        *state = current_settings.enabled && current_settings.*(definition_->enabled_member)
            ? ECS_ENABLED
            : ECS_HIDDEN;
        return S_OK;
    }

    HRESULT general_command::Invoke(IShellItemArray* items, IBindCtx*)
    {
        std::vector<std::filesystem::path> paths;
        const HRESULT result = resolve_selection_or_site(items, site_.Get(), paths);
        return FAILED(result) ? result : launch_runner(definition_->command, paths);
    }

    HRESULT general_command::GetFlags(EXPCMDFLAGS* flags)
    {
        if (flags == nullptr)
        {
            return E_POINTER;
        }
        *flags = ECF_DEFAULT;
        return S_OK;
    }

    HRESULT general_command::EnumSubCommands(IEnumExplorerCommand** commands)
    {
        if (commands == nullptr)
        {
            return E_POINTER;
        }
        *commands = nullptr;
        return E_NOTIMPL;
    }

    HRESULT general_command::SetSite(IUnknown* site)
    {
        site_ = site;
        return S_OK;
    }

    HRESULT general_command::GetSite(REFIID interface_id, void** site)
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

    HRESULT create_git_bash_command(REFIID interface_id, void** object) noexcept
    {
        return create_command(git_bash_definition, interface_id, object);
    }

    HRESULT create_git_gui_command(REFIID interface_id, void** object) noexcept
    {
        return create_command(git_gui_definition, interface_id, object);
    }
}

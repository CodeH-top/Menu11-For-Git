#include "GitChildCommand.h"

#include "CommandSupport.h"
#include "Menu11.Shared/RegistrySettings.h"
#include "Menu11.Shared/RepositoryDetector.h"
#include "SelectionResolver.h"
#include "ShellLauncher.h"

#include <windows.h>

#include <new>
#include <optional>
#include <vector>

namespace menu11::shell
{
    namespace
    {
        [[nodiscard]] bool selections_match_context(
            const git_command_definition& definition,
            const std::vector<std::filesystem::path>& selections) noexcept
        {
            if (definition.context == command_context::any)
            {
                return true;
            }
            if (selections.empty() ||
                (definition.requires_single_selection && selections.size() != 1))
            {
                return false;
            }

            try
            {
                std::optional<std::filesystem::path> shared_repository;
                for (const auto& selection : selections)
                {
                    const auto context = inspect_repository_context(selection);
                    if (!context.selection_exists)
                    {
                        return false;
                    }

                    switch (definition.context)
                    {
                    case command_context::repository:
                        if (selections.size() != 1 || context.selection_is_file || !context.repository_root)
                        {
                            return false;
                        }
                        break;
                    case command_context::file:
                        if (!context.selection_is_file || !context.repository_root)
                        {
                            return false;
                        }
                        if (shared_repository && *shared_repository != *context.repository_root)
                        {
                            return false;
                        }
                        shared_repository = context.repository_root;
                        break;
                    case command_context::non_repository_directory:
                        if (selections.size() != 1 || context.selection_is_file || context.repository_root)
                        {
                            return false;
                        }
                        break;
                    case command_context::any:
                        break;
                    }
                }
                return true;
            }
            catch (...)
            {
                return false;
            }
        }
    }

    git_child_command::git_child_command(
        const git_command_definition& definition,
        IUnknown* site) noexcept
        : definition_(&definition),
          site_(site)
    {
    }

    HRESULT git_child_command::QueryInterface(REFIID interface_id, void** object)
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

    ULONG git_child_command::AddRef()
    {
        return add_reference();
    }

    ULONG git_child_command::Release()
    {
        return release_reference();
    }

    HRESULT git_child_command::GetTitle(IShellItemArray*, PWSTR* title)
    {
        const auto current_settings = registry_settings_store().load();
        return duplicate_string(localize(definition_->title, current_settings.language), title);
    }

    HRESULT git_child_command::GetIcon(IShellItemArray*, PWSTR* icon)
    {
        return get_shell_icon(definition_->icon_resource_id, icon);
    }

    HRESULT git_child_command::GetToolTip(IShellItemArray*, PWSTR* tool_tip)
    {
        return get_no_tool_tip(tool_tip);
    }

    HRESULT git_child_command::GetCanonicalName(GUID* canonical_name)
    {
        if (canonical_name == nullptr)
        {
            return E_POINTER;
        }
        *canonical_name = definition_->canonical_name;
        return S_OK;
    }

    HRESULT git_child_command::GetState(IShellItemArray* items, BOOL, EXPCMDSTATE* state)
    {
        if (state == nullptr)
        {
            return E_POINTER;
        }

        const registry_settings_store settings_store;
        const auto current_settings = settings_store.load();
        *state = current_settings.enabled &&
                is_git_command_enabled(*definition_, current_settings) &&
                is_git_command_visible(*definition_, items, site_.Get())
            ? ECS_ENABLED
            : ECS_HIDDEN;
        return S_OK;
    }

    HRESULT git_child_command::Invoke(IShellItemArray* items, IBindCtx*)
    {
        std::vector<std::filesystem::path> paths;
        if (definition_->context == command_context::any && items == nullptr)
        {
            return launch_runner(definition_->runner, paths);
        }

        const HRESULT result = resolve_selection_or_site(items, site_.Get(), paths);
        if (FAILED(result))
        {
            return result;
        }
        if (!selections_match_context(*definition_, paths))
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
        return launch_runner(definition_->runner, paths);
    }

    HRESULT git_child_command::GetFlags(EXPCMDFLAGS* flags)
    {
        if (flags == nullptr)
        {
            return E_POINTER;
        }
        *flags = definition_->separator_before ? ECF_SEPARATORBEFORE : ECF_DEFAULT;
        return S_OK;
    }

    HRESULT git_child_command::EnumSubCommands(IEnumExplorerCommand** commands)
    {
        if (commands == nullptr)
        {
            return E_POINTER;
        }
        *commands = nullptr;
        return E_NOTIMPL;
    }

    HRESULT git_child_command::SetSite(IUnknown* site)
    {
        site_ = site;
        return S_OK;
    }

    HRESULT git_child_command::GetSite(REFIID interface_id, void** site)
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

    bool is_git_command_enabled(
        const git_command_definition& definition,
        const settings& current_settings) noexcept
    {
        return definition.is_settings
            ? current_settings.show_settings_command
            : current_settings.is_command_enabled(definition.setting);
    }

    bool is_git_command_visible(
        const git_command_definition& definition,
        IShellItemArray* items,
        IUnknown* site) noexcept
    {
        if (definition.context == command_context::any)
        {
            return true;
        }

        std::vector<std::filesystem::path> selections;
        return SUCCEEDED(resolve_selection_or_site(items, site, selections)) &&
            selections_match_context(definition, selections);
    }

    HRESULT create_git_child_command(
        const git_command_definition& definition,
        IUnknown* site,
        REFIID interface_id,
        void** object) noexcept
    {
        if (object == nullptr)
        {
            return E_POINTER;
        }
        *object = nullptr;

        auto* command = new (std::nothrow) git_child_command(definition, site);
        if (command == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        const HRESULT result = command->QueryInterface(interface_id, object);
        command->Release();
        return result;
    }
}

#pragma once

#include "ComObject.h"

#include "Menu11.Shared/RunnerProtocol.h"
#include "Menu11.Shared/Settings.h"
#include "Menu11.Shared/Localization.h"

#include <ocidl.h>
#include <shobjidl_core.h>
#include <wrl/client.h>

namespace menu11::shell
{
    struct general_command_definition
    {
        GUID canonical_name;
        localized_string title;
        runner_command command;
        bool settings::* enabled_member;
        unsigned int icon_resource_id;
    };

    class general_command final : public IExplorerCommand, public IObjectWithSite, private com_object
    {
    public:
        explicit general_command(const general_command_definition& definition) noexcept;

        IFACEMETHODIMP QueryInterface(REFIID interface_id, void** object) override;
        IFACEMETHODIMP_(ULONG) AddRef() override;
        IFACEMETHODIMP_(ULONG) Release() override;
        IFACEMETHODIMP GetTitle(IShellItemArray* items, PWSTR* title) override;
        IFACEMETHODIMP GetIcon(IShellItemArray* items, PWSTR* icon) override;
        IFACEMETHODIMP GetToolTip(IShellItemArray* items, PWSTR* tool_tip) override;
        IFACEMETHODIMP GetCanonicalName(GUID* canonical_name) override;
        IFACEMETHODIMP GetState(IShellItemArray* items, BOOL ok_to_be_slow, EXPCMDSTATE* state) override;
        IFACEMETHODIMP Invoke(IShellItemArray* items, IBindCtx* bind_context) override;
        IFACEMETHODIMP GetFlags(EXPCMDFLAGS* flags) override;
        IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand** commands) override;
        IFACEMETHODIMP SetSite(IUnknown* site) override;
        IFACEMETHODIMP GetSite(REFIID interface_id, void** site) override;

    private:
        const general_command_definition* definition_;
        Microsoft::WRL::ComPtr<IUnknown> site_;
    };

    [[nodiscard]] HRESULT create_git_bash_command(REFIID interface_id, void** object) noexcept;
    [[nodiscard]] HRESULT create_git_gui_command(REFIID interface_id, void** object) noexcept;
}

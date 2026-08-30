#pragma once

#include "ComObject.h"

#include <ocidl.h>
#include <shobjidl_core.h>
#include <wrl/client.h>

namespace menu11::shell
{
    class git_menu_command final : public IExplorerCommand, public IObjectWithSite, private com_object
    {
    public:
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
        Microsoft::WRL::ComPtr<IUnknown> site_;
    };

    [[nodiscard]] HRESULT create_git_menu_command(REFIID interface_id, void** object) noexcept;
}

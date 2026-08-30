#pragma once

#include "ComObject.h"
#include "GitCommandDefinition.h"

#include <shobjidl_core.h>
#include <wrl/client.h>

#include <cstddef>
#include <vector>

namespace menu11::shell
{
    class git_command_enumerator final : public IEnumExplorerCommand, private com_object
    {
    public:
        explicit git_command_enumerator(
            std::vector<const git_command_definition*> definitions,
            IUnknown* site = nullptr,
            std::size_t index = 0) noexcept;

        IFACEMETHODIMP QueryInterface(REFIID interface_id, void** object) override;
        IFACEMETHODIMP_(ULONG) AddRef() override;
        IFACEMETHODIMP_(ULONG) Release() override;
        IFACEMETHODIMP Next(
            ULONG count,
            IExplorerCommand** commands,
            ULONG* fetched) override;
        IFACEMETHODIMP Skip(ULONG count) override;
        IFACEMETHODIMP Reset() override;
        IFACEMETHODIMP Clone(IEnumExplorerCommand** enumerator) override;

    private:
        std::vector<const git_command_definition*> definitions_;
        Microsoft::WRL::ComPtr<IUnknown> site_;
        std::size_t index_ = 0;
    };

    [[nodiscard]] HRESULT create_git_command_enumerator(
        std::vector<const git_command_definition*> definitions,
        IUnknown* site,
        IEnumExplorerCommand** enumerator) noexcept;
}

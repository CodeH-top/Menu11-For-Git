#pragma once

#include "ComObject.h"

#include <unknwn.h>

namespace menu11::shell
{
    using instance_creator = HRESULT (*)(REFIID interface_id, void** object) noexcept;

    class class_factory final : public IClassFactory, private com_object
    {
    public:
        explicit class_factory(instance_creator creator) noexcept;

        IFACEMETHODIMP QueryInterface(REFIID interface_id, void** object) override;
        IFACEMETHODIMP_(ULONG) AddRef() override;
        IFACEMETHODIMP_(ULONG) Release() override;
        IFACEMETHODIMP CreateInstance(IUnknown* outer, REFIID interface_id, void** object) override;
        IFACEMETHODIMP LockServer(BOOL lock) override;

    private:
        instance_creator creator_;
    };
}

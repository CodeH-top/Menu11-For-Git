#include "ClassFactory.h"

#include "Module.h"

namespace menu11::shell
{
    class_factory::class_factory(const instance_creator creator) noexcept
        : creator_(creator)
    {
    }

    HRESULT class_factory::QueryInterface(REFIID interface_id, void** object)
    {
        if (object == nullptr)
        {
            return E_POINTER;
        }
        *object = nullptr;

        if (interface_id == IID_IUnknown || interface_id == IID_IClassFactory)
        {
            *object = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG class_factory::AddRef()
    {
        return add_reference();
    }

    ULONG class_factory::Release()
    {
        return release_reference();
    }

    HRESULT class_factory::CreateInstance(IUnknown* outer, REFIID interface_id, void** object)
    {
        if (outer != nullptr)
        {
            return CLASS_E_NOAGGREGATION;
        }
        return creator_(interface_id, object);
    }

    HRESULT class_factory::LockServer(const BOOL lock)
    {
        if (lock)
        {
            add_module_reference();
        }
        else
        {
            release_module_reference();
        }
        return S_OK;
    }
}

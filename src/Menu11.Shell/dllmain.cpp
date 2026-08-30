#include "ClassFactory.h"
#include "GeneralCommand.h"
#include "GitMenuCommand.h"
#include "Module.h"
#include "ShellGuids.h"

#include <windows.h>

#include <new>

BOOL APIENTRY DllMain(const HMODULE module, const DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        menu11::shell::set_module_instance(module);
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

STDAPI DllCanUnloadNow()
{
    return menu11::shell::module_reference_count() == 0 ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID class_id, REFIID interface_id, void** object)
{
    if (object == nullptr)
    {
        return E_POINTER;
    }

    *object = nullptr;
    menu11::shell::instance_creator creator = nullptr;
    if (class_id == CLSID_Menu11GitBashCommand)
    {
        creator = &menu11::shell::create_git_bash_command;
    }
    else if (class_id == CLSID_Menu11GitGuiCommand)
    {
        creator = &menu11::shell::create_git_gui_command;
    }
    else if (class_id == CLSID_Menu11GitMenuCommand)
    {
        creator = &menu11::shell::create_git_menu_command;
    }
    else
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    auto* factory = new (std::nothrow) menu11::shell::class_factory(creator);
    if (factory == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    const HRESULT result = factory->QueryInterface(interface_id, object);
    factory->Release();
    return result;
}

#include "Menu11.Shared/GitBash.h"
#include "Menu11.Shared/GitDetection.h"
#include "Menu11.Shared/GitGui.h"
#include "Menu11.Shared/GitRepositoryCommands.h"
#include "Menu11.Shared/Localization.h"
#include "Menu11.Shared/ProcessLauncher.h"
#include "Menu11.Shared/RegistrySettings.h"
#include "Menu11.Shared/RepositoryDetector.h"
#include "Menu11.Shared/RunnerProtocol.h"
#include "Menu11.Shared/WindowsCommandLine.h"
#include "Resource.h"

#include <windows.h>
#include <ocidl.h>
#include <shobjidl_core.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shell32.lib")

namespace
{
    int failures = 0;

    void expect(const bool condition, const std::wstring& message)
    {
        if (condition)
        {
            std::wcout << L"[PASS] " << message << L'\n';
            return;
        }

        ++failures;
        std::wcerr << L"[FAIL] " << message << L'\n';
    }

    [[nodiscard]] bool uses_shell_icon_resource(
        const PWSTR icon,
        const unsigned int resource_id)
    {
        if (icon == nullptr)
        {
            return false;
        }

        const auto suffix = L"Menu11.Shell.dll,-" + std::to_wstring(resource_id);
        return std::wstring_view(icon).ends_with(suffix);
    }

    [[nodiscard]] bool uses_git_command_icon_resource(const PWSTR icon)
    {
        for (unsigned int resource_id = IDI_MENU11_STATUS;
            resource_id <= IDI_MENU11_SETTINGS;
            ++resource_id)
        {
            if (uses_shell_icon_resource(icon, resource_id))
            {
                return true;
            }
        }
        return false;
    }

    class temporary_directory final
    {
    public:
        temporary_directory()
        {
            std::error_code error;
            path_ = std::filesystem::temp_directory_path(error) /
                (L"Menu11GitDetectionTests-" + std::to_wstring(GetCurrentProcessId()));
            std::filesystem::remove_all(path_, error);
            std::filesystem::create_directories(path_, error);
        }

        ~temporary_directory()
        {
            std::error_code error;
            std::filesystem::remove_all(path_, error);
        }

        temporary_directory(const temporary_directory&) = delete;
        temporary_directory& operator=(const temporary_directory&) = delete;

        [[nodiscard]] const std::filesystem::path& path() const noexcept
        {
            return path_;
        }

    private:
        std::filesystem::path path_;
    };

    void touch(const std::filesystem::path& path)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream stream(path, std::ios::binary);
        stream.put('\0');
    }

    [[nodiscard]] bool paths_refer_to_same_entry(
        const std::filesystem::path& left,
        const std::filesystem::path& right) noexcept
    {
        // Hosted Windows runners can expose temporary directories through reparse points.
        std::error_code error;
        return std::filesystem::equivalent(left, right, error) && !error;
    }

    [[nodiscard]] bool is_git_bash_directory_argument(
        const std::wstring_view argument,
        const std::filesystem::path& directory) noexcept
    {
        constexpr std::wstring_view prefix = L"--cd=";
        return argument.starts_with(prefix) && paths_refer_to_same_entry(
            std::filesystem::path(argument.substr(prefix.size())),
            directory);
    }

    template <typename function_type>
    [[nodiscard]] function_type load_function(const HMODULE module, const char* name) noexcept
    {
        const FARPROC address = GetProcAddress(module, name);
        static_assert(sizeof(function_type) == sizeof(address));

        function_type function = nullptr;
        std::memcpy(&function, &address, sizeof(function));
        return function;
    }

    class test_shell_site final : public IUnknown
    {
    public:
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID interface_id, void** object) override
        {
            if (object == nullptr)
            {
                return E_POINTER;
            }
            *object = nullptr;
            if (interface_id != IID_IUnknown)
            {
                return E_NOINTERFACE;
            }
            *object = static_cast<IUnknown*>(this);
            AddRef();
            return S_OK;
        }

        ULONG STDMETHODCALLTYPE AddRef() override
        {
            return references_.fetch_add(1, std::memory_order_relaxed) + 1;
        }

        ULONG STDMETHODCALLTYPE Release() override
        {
            const ULONG remaining = references_.fetch_sub(1, std::memory_order_acq_rel) - 1;
            if (remaining == 0)
            {
                delete this;
            }
            return remaining;
        }

    private:
        std::atomic_ulong references_ = 1;
    };

    void test_shell_site_contract(IExplorerCommand* command, const std::wstring_view name)
    {
        Microsoft::WRL::ComPtr<IObjectWithSite> site_aware;
        const HRESULT query_result = command->QueryInterface(IID_PPV_ARGS(&site_aware));
        expect(query_result == S_OK && site_aware != nullptr,
            std::wstring(name) + L" exposes IObjectWithSite");
        if (site_aware == nullptr)
        {
            return;
        }

        void* missing_site = &failures;
        expect(site_aware->GetSite(IID_IUnknown, &missing_site) == E_FAIL && missing_site == nullptr,
            std::wstring(name) + L" starts without a stale Explorer site");

        Microsoft::WRL::ComPtr<IUnknown> test_site;
        test_site.Attach(new (std::nothrow) test_shell_site());
        expect(test_site != nullptr, std::wstring(name) + L" creates a test site object");
        if (test_site != nullptr)
        {
            expect(site_aware->SetSite(test_site.Get()) == S_OK,
                std::wstring(name) + L" accepts an Explorer site");
            Microsoft::WRL::ComPtr<IUnknown> returned_site;
            expect(site_aware->GetSite(IID_PPV_ARGS(&returned_site)) == S_OK &&
                    returned_site.Get() == test_site.Get(),
                std::wstring(name) + L" returns the same Explorer site");
        }

        expect(site_aware->SetSite(nullptr) == S_OK,
            std::wstring(name) + L" releases its Explorer site");
        missing_site = &failures;
        expect(site_aware->GetSite(IID_IUnknown, &missing_site) == E_FAIL && missing_site == nullptr,
            std::wstring(name) + L" clears the Explorer site deterministically");
    }

    int query_registered_shell_title()
    {
        constexpr GUID git_bash_command_id{
            0x95782209,
            0xBC84,
            0x40EB,
            {0xBC, 0x3B, 0xCF, 0x8A, 0x25, 0xDF, 0xC5, 0xF3},
        };

        const HRESULT initialize_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(initialize_result))
        {
            return 30;
        }

        IExplorerCommand* command = nullptr;
        const HRESULT activation_result = CoCreateInstance(
            git_bash_command_id,
            nullptr,
            CLSCTX_LOCAL_SERVER,
            __uuidof(IExplorerCommand),
            reinterpret_cast<void**>(&command));
        if (FAILED(activation_result) || command == nullptr)
        {
            std::wcerr << L"CoCreateInstance failed: 0x" << std::hex << activation_result << L'\n';
            CoUninitialize();
            return 31;
        }

        PWSTR title = nullptr;
        const HRESULT title_result = command->GetTitle(nullptr, &title);
        command->Release();
        if (FAILED(title_result) || title == nullptr)
        {
            CoTaskMemFree(title);
            CoUninitialize();
            return 32;
        }

        std::wcout << title << L'\n';
        CoTaskMemFree(title);
        CoUninitialize();
        return 0;
    }

    void test_shell_extension(
        const std::filesystem::path& shell_path,
        const bool invoke_settings = false)
    {
        using get_class_object_function = HRESULT(STDAPICALLTYPE*)(REFCLSID, REFIID, void**);
        using can_unload_now_function = HRESULT(STDAPICALLTYPE*)();

        constexpr GUID git_bash_command_id{
            0x95782209,
            0xBC84,
            0x40EB,
            {0xBC, 0x3B, 0xCF, 0x8A, 0x25, 0xDF, 0xC5, 0xF3},
        };
        constexpr GUID git_gui_command_id{
            0xE6B96683,
            0xEE0A,
            0x46DC,
            {0xA9, 0x0A, 0x8D, 0xAE, 0x7D, 0xCE, 0xE3, 0xB6},
        };
        constexpr GUID git_menu_command_id{
            0x1AE9CDF7,
            0xBF8D,
            0x41FD,
            {0xAE, 0xB2, 0x9B, 0x34, 0x6D, 0x34, 0x31, 0xD4},
        };

        const HMODULE shell_module = LoadLibraryExW(
            shell_path.c_str(),
            nullptr,
            LOAD_WITH_ALTERED_SEARCH_PATH);
        expect(shell_module != nullptr, L"loads the native Shell extension DLL");
        if (shell_module == nullptr)
        {
            return;
        }

        const auto get_class_object = load_function<get_class_object_function>(
            shell_module,
            "DllGetClassObject");
        const auto can_unload_now = load_function<can_unload_now_function>(
            shell_module,
            "DllCanUnloadNow");
        expect(get_class_object != nullptr && can_unload_now != nullptr,
            L"exports the standard COM entry points");
        if (get_class_object == nullptr || can_unload_now == nullptr)
        {
            FreeLibrary(shell_module);
            return;
        }

        expect(can_unload_now() == S_OK, L"starts with no live Shell COM objects");

        void* unavailable_object = &failures;
        expect(get_class_object(GUID_NULL, IID_IClassFactory, &unavailable_object) ==
                CLASS_E_CLASSNOTAVAILABLE && unavailable_object == nullptr,
            L"rejects an unregistered Shell COM class");

        void* factory_object = nullptr;
        const HRESULT factory_result = get_class_object(
            git_bash_command_id,
            IID_IClassFactory,
            &factory_object);
        auto* factory = static_cast<IClassFactory*>(factory_object);
        expect(factory_result == S_OK && factory != nullptr,
            L"creates the Git Bash class factory");
        if (factory == nullptr)
        {
            FreeLibrary(shell_module);
            return;
        }

        expect(can_unload_now() == S_FALSE, L"keeps the Shell DLL loaded while its class factory is live");
        expect(factory->LockServer(TRUE) == S_OK && can_unload_now() == S_FALSE,
            L"honors COM server locks");
        expect(factory->LockServer(FALSE) == S_OK, L"releases COM server locks");

        void* command_object = nullptr;
        const HRESULT command_result = factory->CreateInstance(
            nullptr,
            __uuidof(IExplorerCommand),
            &command_object);
        auto* command = static_cast<IExplorerCommand*>(command_object);
        expect(command_result == S_OK && command != nullptr,
            L"creates the Git Bash IExplorerCommand");
        factory->Release();

        if (command != nullptr)
        {
            test_shell_site_contract(command, L"Git Bash command");

            PWSTR title = nullptr;
            const HRESULT title_result = command->GetTitle(nullptr, &title);
            const auto expected_title = menu11::localize(
                menu11::localized_string::git_bash_here,
                menu11::registry_settings_store().load().language);
            expect(title_result == S_OK && title != nullptr && std::wstring_view(title) == expected_title,
                L"exposes the expected modern-menu title");
            CoTaskMemFree(title);

            GUID canonical_name = GUID_NULL;
            expect(command->GetCanonicalName(&canonical_name) == S_OK &&
                    IsEqualGUID(canonical_name, git_bash_command_id),
                L"exposes the registered canonical command GUID");

            EXPCMDSTATE state = ECS_DISABLED;
            expect(command->GetState(nullptr, FALSE, &state) == S_OK &&
                    (state == ECS_ENABLED || state == ECS_HIDDEN),
                L"returns a valid registry-backed command state");

            PWSTR icon = nullptr;
            const HRESULT icon_result = command->GetIcon(nullptr, &icon);
            expect((icon_result == S_OK && uses_shell_icon_resource(
                        icon, IDI_MENU11_GIT_BASH)) ||
                    (icon_result == E_NOTIMPL && icon == nullptr),
                L"returns the dedicated Git Bash icon when icons are enabled");
            CoTaskMemFree(icon);

            EXPCMDFLAGS flags = ECF_HASSUBCOMMANDS;
            expect(command->GetFlags(&flags) == S_OK && flags == ECF_DEFAULT,
                L"marks Git Bash as a leaf command");

            IEnumExplorerCommand* subcommands = nullptr;
            expect(command->EnumSubCommands(&subcommands) == E_NOTIMPL && subcommands == nullptr,
                L"does not expose a third menu level from Git Bash");

            command->Release();
        }

        void* gui_factory_object = nullptr;
        const HRESULT gui_factory_result = get_class_object(
            git_gui_command_id,
            IID_IClassFactory,
            &gui_factory_object);
        auto* gui_factory = static_cast<IClassFactory*>(gui_factory_object);
        expect(gui_factory_result == S_OK && gui_factory != nullptr,
            L"creates the Git GUI class factory");
        if (gui_factory != nullptr)
        {
            void* gui_command_object = nullptr;
            const HRESULT gui_command_result = gui_factory->CreateInstance(
                nullptr,
                __uuidof(IExplorerCommand),
                &gui_command_object);
            auto* gui_command = static_cast<IExplorerCommand*>(gui_command_object);
            expect(gui_command_result == S_OK && gui_command != nullptr,
                L"creates the Git GUI IExplorerCommand");
            gui_factory->Release();

            if (gui_command != nullptr)
            {
                test_shell_site_contract(gui_command, L"Git GUI command");

                PWSTR title = nullptr;
                const HRESULT title_result = gui_command->GetTitle(nullptr, &title);
                const auto expected_title = menu11::localize(
                    menu11::localized_string::git_gui_here,
                    menu11::registry_settings_store().load().language);
                expect(title_result == S_OK && title != nullptr && std::wstring_view(title) == expected_title,
                    L"exposes the Git GUI modern-menu title");
                CoTaskMemFree(title);

                GUID canonical_name = GUID_NULL;
                expect(gui_command->GetCanonicalName(&canonical_name) == S_OK &&
                        IsEqualGUID(canonical_name, git_gui_command_id),
                    L"exposes the registered Git GUI command GUID");

                EXPCMDSTATE state = ECS_DISABLED;
                expect(gui_command->GetState(nullptr, FALSE, &state) == S_OK &&
                        (state == ECS_ENABLED || state == ECS_HIDDEN),
                    L"returns a valid registry-backed Git GUI state");

                PWSTR icon = nullptr;
                const HRESULT icon_result = gui_command->GetIcon(nullptr, &icon);
                expect((icon_result == S_OK && uses_shell_icon_resource(
                            icon, IDI_MENU11_GIT_GUI)) ||
                        (icon_result == E_NOTIMPL && icon == nullptr),
                    L"returns the dedicated Git GUI icon when icons are enabled");
                CoTaskMemFree(icon);

                EXPCMDFLAGS flags = ECF_HASSUBCOMMANDS;
                expect(gui_command->GetFlags(&flags) == S_OK && flags == ECF_DEFAULT,
                    L"marks Git GUI as a leaf command");
                gui_command->Release();
            }
        }

        void* menu_factory_object = nullptr;
        const HRESULT menu_factory_result = get_class_object(
            git_menu_command_id,
            IID_IClassFactory,
            &menu_factory_object);
        auto* menu_factory = static_cast<IClassFactory*>(menu_factory_object);
        expect(menu_factory_result == S_OK && menu_factory != nullptr,
            L"creates the Git menu class factory");
        if (menu_factory != nullptr)
        {
            void* menu_command_object = nullptr;
            const HRESULT menu_command_result = menu_factory->CreateInstance(
                nullptr,
                __uuidof(IExplorerCommand),
                &menu_command_object);
            auto* menu_command = static_cast<IExplorerCommand*>(menu_command_object);
            expect(menu_command_result == S_OK && menu_command != nullptr,
                L"creates the Git parent IExplorerCommand");
            menu_factory->Release();

            if (menu_command != nullptr)
            {
                test_shell_site_contract(menu_command, L"Git parent command");

                PWSTR title = nullptr;
                const HRESULT title_result = menu_command->GetTitle(nullptr, &title);
                expect(title_result == S_OK && title != nullptr && std::wstring_view(title) == L"Git",
                    L"exposes the Git parent modern-menu title");
                CoTaskMemFree(title);

                GUID canonical_name = GUID_NULL;
                expect(menu_command->GetCanonicalName(&canonical_name) == S_OK &&
                        IsEqualGUID(canonical_name, git_menu_command_id),
                    L"exposes the registered Git parent command GUID");

                EXPCMDSTATE state = ECS_DISABLED;
                expect(menu_command->GetState(nullptr, FALSE, &state) == S_OK &&
                        (state == ECS_ENABLED || state == ECS_HIDDEN),
                    L"returns a valid registry-backed Git parent state");

                PWSTR icon = nullptr;
                const HRESULT icon_result = menu_command->GetIcon(nullptr, &icon);
                expect((icon_result == S_OK && uses_shell_icon_resource(
                            icon, IDI_MENU11_GIT)) ||
                        (icon_result == E_NOTIMPL && icon == nullptr),
                    L"returns the dedicated Git graph icon when icons are enabled");
                CoTaskMemFree(icon);

                EXPCMDFLAGS flags = ECF_DEFAULT;
                expect(menu_command->GetFlags(&flags) == S_OK && flags == ECF_HASSUBCOMMANDS,
                    L"marks Git as the one allowed submenu parent");
                expect(menu_command->Invoke(nullptr, nullptr) == E_NOTIMPL,
                    L"does not invoke work from the submenu parent");

                Microsoft::WRL::ComPtr<IObjectWithSite> menu_site_aware;
                Microsoft::WRL::ComPtr<IUnknown> propagated_site;
                expect(menu_command->QueryInterface(IID_PPV_ARGS(&menu_site_aware)) == S_OK &&
                        menu_site_aware != nullptr,
                    L"queries the Git parent site interface for child propagation");
                propagated_site.Attach(new (std::nothrow) test_shell_site());
                expect(propagated_site != nullptr, L"creates the Git child propagation site");
                if (menu_site_aware != nullptr && propagated_site != nullptr)
                {
                    expect(menu_site_aware->SetSite(propagated_site.Get()) == S_OK,
                        L"sets the site before Git child enumeration");
                }

                IEnumExplorerCommand* subcommands = nullptr;
                expect(menu_command->EnumSubCommands(&subcommands) == S_OK && subcommands != nullptr,
                    L"creates the flat Git child command enumerator");
                if (subcommands != nullptr)
                {
                    const auto current_settings = menu11::registry_settings_store().load();
                    const auto expected_children = current_settings.enabled
                        ? std::popcount(current_settings.enabled_commands & menu11::all_git_commands_mask) +
                            static_cast<int>(current_settings.show_settings_command)
                        : 0;
                    const auto settings_title = menu11::localize(
                        menu11::localized_string::settings,
                        current_settings.language);

                    IExplorerCommand* invalid_batch[2]{};
                    expect(subcommands->Next(2, invalid_batch, nullptr) == E_POINTER,
                        L"enforces the COM fetched-count contract for batched enumeration");

                    std::vector<GUID> child_names;
                    std::vector<std::wstring> child_icons;
                    std::wstring last_title;
                    bool verified_child_site = false;
                    for (;;)
                    {
                        IExplorerCommand* child = nullptr;
                        ULONG fetched = 0;
                        const HRESULT next_result = subcommands->Next(1, &child, &fetched);
                        if (next_result == S_FALSE)
                        {
                            expect(child == nullptr && fetched == 0,
                                L"ends Git child enumeration cleanly");
                            break;
                        }

                        expect(next_result == S_OK && child != nullptr && fetched == 1,
                            L"enumerates one enabled Git child command");
                        if (child == nullptr)
                        {
                            break;
                        }

                        if (!verified_child_site && propagated_site != nullptr)
                        {
                            Microsoft::WRL::ComPtr<IObjectWithSite> child_site_aware;
                            expect(child->QueryInterface(IID_PPV_ARGS(&child_site_aware)) == S_OK &&
                                    child_site_aware != nullptr,
                                L"Git children expose IObjectWithSite");
                            if (child_site_aware != nullptr)
                            {
                                Microsoft::WRL::ComPtr<IUnknown> child_site;
                                expect(child_site_aware->GetSite(IID_PPV_ARGS(&child_site)) == S_OK &&
                                        child_site.Get() == propagated_site.Get(),
                                    L"Git children inherit the parent Explorer site");
                                verified_child_site = child_site.Get() == propagated_site.Get();
                            }
                        }

                        PWSTR child_title = nullptr;
                        expect(child->GetTitle(nullptr, &child_title) == S_OK && child_title != nullptr,
                            L"each Git child has a title");
                        if (child_title != nullptr)
                        {
                            last_title = child_title;
                            if (invoke_settings && last_title == settings_title)
                            {
                                expect(child->Invoke(nullptr, nullptr) == S_OK,
                                    L"invokes Settings through the Shell command chain");
                            }
                            CoTaskMemFree(child_title);
                        }

                        GUID child_name = GUID_NULL;
                        expect(child->GetCanonicalName(&child_name) == S_OK && child_name != GUID_NULL,
                            L"each Git child has a canonical GUID");
                        child_names.push_back(child_name);

                        PWSTR child_icon = nullptr;
                        const HRESULT child_icon_result = child->GetIcon(nullptr, &child_icon);
                        const bool child_icon_is_valid =
                            child_icon_result == E_NOTIMPL && child_icon == nullptr ||
                            child_icon_result == S_OK && uses_git_command_icon_resource(child_icon);
                        expect(child_icon_is_valid, L"each Git child returns its dedicated icon resource");
                        if (child_icon != nullptr)
                        {
                            child_icons.emplace_back(child_icon);
                        }
                        CoTaskMemFree(child_icon);

                        EXPCMDFLAGS child_flags = ECF_HASSUBCOMMANDS;
                        expect(child->GetFlags(&child_flags) == S_OK &&
                                (child_flags & ECF_HASSUBCOMMANDS) == 0,
                            L"every Git child is a leaf command");
                        IEnumExplorerCommand* third_level = nullptr;
                        expect(child->EnumSubCommands(&third_level) == E_NOTIMPL && third_level == nullptr,
                            L"Git children cannot create a third menu level");
                        child->Release();
                    }

                    expect(child_names.size() == static_cast<std::size_t>(expected_children),
                        L"enumerates exactly the enabled Registry-backed Git commands");
                    if (expected_children != 0 && propagated_site != nullptr)
                    {
                        expect(verified_child_site,
                            L"verifies Explorer site propagation on an enumerated Git child");
                    }
                    if (current_settings.enabled && current_settings.show_settings_command)
                    {
                        expect(last_title == settings_title, L"keeps Settings at the bottom of the Git menu");
                    }

                    bool canonical_names_are_unique = true;
                    for (std::size_t left = 0; left < child_names.size(); ++left)
                    {
                        for (std::size_t right = left + 1; right < child_names.size(); ++right)
                        {
                            canonical_names_are_unique = canonical_names_are_unique &&
                                !IsEqualGUID(child_names[left], child_names[right]);
                        }
                    }
                    expect(canonical_names_are_unique, L"uses a unique canonical GUID for every Git child");
                    if (current_settings.show_icons && child_icons.size() > 1)
                    {
                        std::sort(child_icons.begin(), child_icons.end());
                        const auto distinct_end = std::unique(child_icons.begin(), child_icons.end());
                        expect(std::distance(child_icons.begin(), distinct_end) > 1,
                            L"uses command-specific resources instead of one repeated Shell icon");
                    }

                    expect(subcommands->Reset() == S_OK, L"resets Git child enumeration");
                    IEnumExplorerCommand* clone = nullptr;
                    expect(subcommands->Clone(&clone) == S_OK && clone != nullptr,
                        L"clones Git child enumeration state");
                    if (clone != nullptr && expected_children != 0)
                    {
                        IExplorerCommand* original_child = nullptr;
                        IExplorerCommand* cloned_child = nullptr;
                        expect(subcommands->Next(1, &original_child, nullptr) == S_OK &&
                                clone->Next(1, &cloned_child, nullptr) == S_OK,
                            L"a cloned enumerator resumes at the same command");
                        if (original_child != nullptr && cloned_child != nullptr)
                        {
                            GUID original_name = GUID_NULL;
                            GUID cloned_name = GUID_NULL;
                            original_child->GetCanonicalName(&original_name);
                            cloned_child->GetCanonicalName(&cloned_name);
                            expect(IsEqualGUID(original_name, cloned_name),
                                L"cloned enumeration returns the same canonical command");
                        }
                        if (original_child != nullptr)
                        {
                            original_child->Release();
                        }
                        if (cloned_child != nullptr)
                        {
                            cloned_child->Release();
                        }
                    }
                    if (clone != nullptr)
                    {
                        clone->Release();
                    }
                    subcommands->Release();
                }
                if (menu_site_aware != nullptr)
                {
                    expect(menu_site_aware->SetSite(nullptr) == S_OK,
                        L"releases the Git parent site after child enumeration");
                }
                menu_command->Release();
            }
        }

        expect(can_unload_now() == S_OK, L"becomes unloadable after all Shell COM objects are released");
        expect(FreeLibrary(shell_module) != FALSE, L"unloads the Shell extension cleanly");
    }
}

int wmain(const int argument_count, wchar_t** argument_values)
{
    if (argument_count == 2 &&
        std::wstring_view(argument_values[1]) == L"--query-registered-shell-title")
    {
        return query_registered_shell_title();
    }

    if (argument_count == 3 &&
        std::wstring_view(argument_values[1]) == L"--invoke-settings-shell")
    {
        test_shell_extension(argument_values[2], true);
        return failures == 0 ? 0 : 1;
    }

    if (argument_count > 1 && std::wstring_view(argument_values[1]) == L"--child-verify")
    {
        const std::vector<std::wstring_view> expected{
            L"space path",
            L"符号 & | ^ %",
            L"quote\"inside",
            L"trailing\\",
            L"",
        };
        if (argument_count != static_cast<int>(expected.size()) + 2)
        {
            return 20;
        }
        for (std::size_t index = 0; index < expected.size(); ++index)
        {
            if (argument_values[index + 2] != expected[index])
            {
                return 21;
            }
        }
        return 0;
    }

    temporary_directory temporary;
    const auto root = temporary.path() / L"Portable Git 测试";
    touch(root / L"cmd" / L"git.exe");
    touch(root / L"git-bash.exe");
    touch(root / L"bin" / L"bash.exe");
    touch(root / L"cmd" / L"git-gui.exe");

    const auto from_root = menu11::git::inspect_candidate(root, menu11::git::detection_source::user_selected);
    expect(from_root.has_value(), L"accepts a Git for Windows installation root");
    if (from_root)
    {
        expect(from_root->git_executable == std::filesystem::weakly_canonical(root / L"cmd" / L"git.exe"),
            L"prefers cmd\\git.exe");
        expect(from_root->has_git_bash() &&
                from_root->git_bash_executable == std::filesystem::weakly_canonical(root / L"git-bash.exe"),
            L"finds git-bash.exe for Git Bash Here");
        expect(from_root->has_bash() &&
                from_root->bash_executable == std::filesystem::weakly_canonical(root / L"bin" / L"bash.exe"),
            L"finds bin\\bash.exe for fixed command scripts");
        expect(from_root->has_gui(), L"finds cmd\\git-gui.exe");
    }

    const auto from_executable = menu11::git::inspect_candidate(
        root / L"cmd" / L"git.exe",
        menu11::git::detection_source::path_environment);
    expect(from_executable.has_value() && from_executable->install_root == std::filesystem::weakly_canonical(root),
        L"derives the install root from an explicit cmd\\git.exe path");

    const auto invalid = menu11::git::inspect_candidate(
        temporary.path() / L"NotGit",
        menu11::git::detection_source::user_selected);
    expect(!invalid.has_value(), L"rejects a missing candidate");

    const auto detected = menu11::git::detect_git_for_windows();
    if (detected)
    {
        expect(std::filesystem::is_regular_file(detected->git_executable), L"system detection returns an existing git.exe");
        expect(detected->has_git_bash() && std::filesystem::is_regular_file(detected->git_bash_executable),
            L"system detection returns an existing git-bash.exe");
        expect(detected->has_bash() && std::filesystem::is_regular_file(detected->bash_executable),
            L"system detection returns an existing bin\\bash.exe");
        std::wcout << L"[INFO] Git root: " << detected->install_root << L'\n'
                   << L"[INFO] Git version: " << detected->version << L'\n'
                   << L"[INFO] Detection source: " << menu11::git::source_name(detected->source) << L'\n';

        if (detected->has_bash())
        {
            const std::wstring hostile_argument = L"spaces & | ^ % ! ( ) Unicode-参数 trailing\\";
            const menu11::process_launch_options probe_options{
                .executable = detected->bash_executable,
                .arguments = {
                    L"--login",
                    L"-c",
                    L"test \"$#\" -eq 2 && test \"$1\" = \"$2\" && command -v git >/dev/null",
                    L"menu11",
                    hostile_argument,
                    hostile_argument,
                },
                .working_directory = temporary.path(),
                .show_window = SW_HIDE,
                .wait_for_exit = true,
            };
            const auto probe_result = menu11::launch_process(probe_options);
            expect(probe_result.started() && probe_result.exit_code == DWORD{0},
                L"real bin\\bash.exe preserves positional arguments and exposes Git on PATH");
        }
    }
    else
    {
        std::wcout << L"[SKIP] Git for Windows is not installed on this test machine.\n";
    }

    const std::wstring test_key = L"Software\\Menu11ForGit\\Tests\\Native-" +
        std::to_wstring(GetCurrentProcessId());
    const menu11::registry_settings_store settings_store(HKEY_CURRENT_USER, test_key);
    expect(settings_store.remove() == ERROR_SUCCESS, L"cleans any stale native settings test key");

    const auto defaults = settings_store.load();
    expect(defaults.enabled && defaults.show_git_bash && defaults.show_git_gui,
        L"uses enabled General defaults when the settings key is absent");
    expect(defaults.enabled_commands == menu11::default_git_commands_mask,
        L"uses the specified default Git command mask");
    expect(defaults.language == menu11::display_language::system,
        L"uses the system language by default");
    expect(menu11::localize(
            menu11::localized_string::settings,
            menu11::display_language::english) == L"Settings" &&
        menu11::localize(
            menu11::localized_string::settings,
            menu11::display_language::simplified_chinese) == L"设置",
        L"localizes native command titles in English and Simplified Chinese");
    expect(menu11::localize(
            menu11::localized_string::status,
            menu11::display_language::simplified_chinese) == L"Status" &&
        menu11::localize(
            menu11::localized_string::init_repository,
            menu11::display_language::simplified_chinese) == L"Init",
        L"keeps standard Git command names in their original form");

    menu11::settings custom;
    custom.enabled = false;
    custom.show_git_bash = false;
    custom.show_git_gui = true;
    custom.enabled_commands = static_cast<std::uint32_t>(menu11::git_command::status) |
        static_cast<std::uint32_t>(menu11::git_command::file_restore);
    custom.show_settings_command = false;
    custom.show_icons = false;
    custom.git_path = std::filesystem::path(L"C:\\工具\\PortableGit");
    custom.language = menu11::display_language::simplified_chinese;
    expect(settings_store.save(custom) == ERROR_SUCCESS, L"writes native settings to the test subkey");

    const auto loaded = settings_store.load();
    expect(!loaded.enabled && !loaded.show_git_bash && loaded.show_git_gui,
        L"round-trips native boolean settings");
    expect(loaded.is_command_enabled(menu11::git_command::status) &&
            loaded.is_command_enabled(menu11::git_command::file_restore) &&
            !loaded.is_command_enabled(menu11::git_command::pull),
        L"round-trips the native command mask");
    expect(loaded.git_path == custom.git_path, L"round-trips a Unicode Git path");
    expect(loaded.language == menu11::display_language::simplified_chinese,
        L"round-trips the native language preference");
    expect(settings_store.remove() == ERROR_SUCCESS, L"removes the native settings test key");
    expect(settings_store.load().enabled_commands == menu11::default_git_commands_mask,
        L"restores native defaults after key removal");

    const auto repository = temporary.path() / L"仓库";
    const auto nested_directory = repository / L"src" / L"子目录";
    const auto selected_file = nested_directory / L"文件 & 空格.txt";
    const auto second_selected_file = nested_directory / L"二号文件 (测试).txt";
    std::filesystem::create_directories(repository / L".git");
    touch(selected_file);
    touch(second_selected_file);

    const auto directory_root = menu11::find_repository_root(nested_directory);
    expect(directory_root && paths_refer_to_same_entry(*directory_root, repository),
        L"finds a parent repository from a Unicode directory");
    const auto file_context = menu11::inspect_repository_context(selected_file);
    expect(file_context.selection_exists && file_context.selection_is_file,
        L"classifies a selected file without invoking Git");
    expect(paths_refer_to_same_entry(file_context.working_directory, nested_directory),
        L"uses the containing directory for a file selection");
    expect(file_context.repository_root &&
            paths_refer_to_same_entry(*file_context.repository_root, repository),
        L"finds a parent repository from a file selection");

    const auto worktree = temporary.path() / L"worktree";
    const auto worktree_child = worktree / L"nested";
    std::filesystem::create_directories(worktree_child);
    touch(worktree / L".git");
    const auto worktree_root = menu11::find_repository_root(worktree_child);
    expect(worktree_root && paths_refer_to_same_entry(*worktree_root, worktree),
        L"recognizes a worktree .git file");

    const auto nested_repository = nested_directory / L"embedded";
    std::filesystem::create_directories(nested_repository / L".git");
    const auto nested_repository_child = nested_repository / L"deep";
    std::filesystem::create_directories(nested_repository_child);
    const auto detected_nested_repository = menu11::find_repository_root(nested_repository_child);
    expect(detected_nested_repository &&
            paths_refer_to_same_entry(*detected_nested_repository, nested_repository),
        L"returns the nearest repository root");

    const auto non_repository = temporary.path() / L"plain";
    std::filesystem::create_directories(non_repository);
    expect(!menu11::find_repository_root(non_repository).has_value(),
        L"returns no repository for an unrelated directory");
    expect(!menu11::inspect_repository_context(temporary.path() / L"missing").selection_exists,
        L"fails safely for a missing selection");

    const std::vector<std::wstring> round_trip_arguments{
        L"simple",
        L"space path",
        L"",
        L"quote\"inside",
        L"trailing\\",
        L"符号 & | ^ %",
    };
    const auto command_line = menu11::build_windows_command_line(
        L"C:\\Program Files\\Menu11ForGit\\Menu11.Runner.exe",
        round_trip_arguments);
    int parsed_count = 0;
    wchar_t** parsed_values = CommandLineToArgvW(command_line.c_str(), &parsed_count);
    expect(parsed_values != nullptr && parsed_count == static_cast<int>(round_trip_arguments.size()) + 1,
        L"builds a command line that CommandLineToArgvW can parse");
    if (parsed_values != nullptr)
    {
        bool all_arguments_match = parsed_count == static_cast<int>(round_trip_arguments.size()) + 1;
        for (std::size_t index = 0; all_arguments_match && index < round_trip_arguments.size(); ++index)
        {
            all_arguments_match = parsed_values[index + 1] == round_trip_arguments[index];
        }
        expect(all_arguments_match, L"round-trips spaces, quotes, slashes, shell characters, and Unicode");
        LocalFree(parsed_values);
    }

    const std::vector<std::wstring_view> valid_protocol_arguments{
        L"--command", L"add", L"--path", L"C:\\仓库\\a & b.txt", L"--path", L"C:\\仓库\\二.txt",
    };
    const auto valid_request = menu11::parse_runner_arguments(valid_protocol_arguments);
    expect(valid_request && valid_request.request->command == menu11::runner_command::file_add &&
            valid_request.request->selections.size() == 2,
        L"parses a multi-selection Runner request");
    const std::vector<std::wstring_view> invalid_protocol_arguments{L"--command", L"status"};
    expect(menu11::parse_runner_arguments(invalid_protocol_arguments).error ==
            menu11::runner_parse_error::missing_selection,
        L"rejects a Runner request without a required selection");
    const std::vector<std::wstring_view> gui_protocol_arguments{
        L"--command", L"gui", L"--path", L"C:\\仓库\\工作区",
    };
    const auto gui_request = menu11::parse_runner_arguments(gui_protocol_arguments);
    expect(gui_request && gui_request.request->command == menu11::runner_command::git_gui,
        L"parses a Git GUI Runner request");
    const std::vector<std::wstring_view> settings_protocol_arguments{
        L"--command", L"settings",
    };
    const auto settings_request = menu11::parse_runner_arguments(settings_protocol_arguments);
    expect(settings_request && settings_request.request->command == menu11::runner_command::settings &&
            settings_request.request->selections.empty(),
        L"parses a Settings Runner request without a selection");

    std::vector<wchar_t> module_path(32768, L'\0');
    const DWORD module_length = GetModuleFileNameW(nullptr, module_path.data(), static_cast<DWORD>(module_path.size()));
    expect(module_length > 0 && module_length < module_path.size(), L"resolves the native test executable path");
    expect(menu11::current_process_directory() == std::filesystem::path(module_path.data()).parent_path(),
        L"resolves the current process directory for adjacent application launch");
    if (module_length > 0 && module_length < module_path.size())
    {
        menu11::process_launch_options child_options{
            .executable = std::filesystem::path(module_path.data()),
            .arguments = {
                L"--child-verify",
                L"space path",
                L"符号 & | ^ %",
                L"quote\"inside",
                L"trailing\\",
                L"",
            },
            .working_directory = temporary.path(),
            .wait_for_exit = true,
        };
        const auto child_result = menu11::launch_process(child_options);
        expect(child_result.started() && child_result.exit_code == DWORD{0},
            L"CreateProcessW preserves hostile-looking arguments without invoking a shell");

        menu11::git::installation fake_git{
            .install_root = std::filesystem::path(module_path.data()).parent_path(),
            .git_executable = std::filesystem::path(module_path.data()),
            .git_bash_executable = std::filesystem::path(module_path.data()),
            .bash_executable = std::filesystem::path(module_path.data()),
            .gui_executable = std::filesystem::path(module_path.data()),
            .version = L"test",
            .source = menu11::git::detection_source::user_selected,
        };
        const auto file_bash_plan = menu11::git::create_bash_launch_plan(fake_git, selected_file);
        expect(file_bash_plan && paths_refer_to_same_entry(
                file_bash_plan.options->working_directory,
                nested_directory),
            L"Git Bash uses a selected file's containing directory");
        if (file_bash_plan)
        {
            expect(file_bash_plan.options->executable == fake_git.git_bash_executable &&
                    file_bash_plan.options->arguments.size() == 1 &&
                    is_git_bash_directory_argument(
                        file_bash_plan.options->arguments.front(),
                        nested_directory),
                L"Git Bash receives one supported --cd argument");
            expect(!file_bash_plan.options->wait_for_exit,
                L"Runner does not remain resident after Git Bash starts");
        }

        const auto directory_bash_plan = menu11::git::create_bash_launch_plan(fake_git, nested_directory);
        expect(directory_bash_plan && paths_refer_to_same_entry(
                directory_bash_plan.options->working_directory,
                nested_directory),
            L"Git Bash uses a selected directory directly");

        const auto file_gui_plan = menu11::git::create_gui_launch_plan(fake_git, selected_file);
        expect(file_gui_plan && paths_refer_to_same_entry(
                file_gui_plan.options->working_directory,
                nested_directory),
            L"Git GUI uses a selected file's containing directory");
        if (file_gui_plan)
        {
            expect(file_gui_plan.options->arguments.empty(),
                L"Git GUI starts without shell-composed arguments");
            expect(!file_gui_plan.options->wait_for_exit,
                L"Runner does not remain resident after Git GUI starts");
        }

        const auto directory_gui_plan = menu11::git::create_gui_launch_plan(fake_git, nested_directory);
        expect(directory_gui_plan && paths_refer_to_same_entry(
                directory_gui_plan.options->working_directory,
                nested_directory),
            L"Git GUI uses a selected directory directly");

        auto git_without_gui = fake_git;
        git_without_gui.gui_executable.clear();
        expect(menu11::git::create_gui_launch_plan(git_without_gui, nested_directory).error ==
                menu11::git::gui_plan_error::executable_unavailable,
            L"Git GUI plan rejects an installation without git-gui.exe");

        const std::vector<std::pair<menu11::runner_command, std::wstring_view>> terminal_commands{
            {menu11::runner_command::status, L"git status;"},
            {menu11::runner_command::pull, L"git pull;"},
            {menu11::runner_command::fetch, L"git fetch;"},
            {menu11::runner_command::push, L"git push;"},
            {menu11::runner_command::repository_log, L"git log --graph"},
            {menu11::runner_command::branch, L"git branch --all;"},
            {menu11::runner_command::stash, L"git stash push;"},
        };
        for (const auto& [command, expected_script_prefix] : terminal_commands)
        {
            const auto plan = menu11::git::create_repository_launch_plan(
                fake_git,
                command,
                nested_directory);
            expect(plan && paths_refer_to_same_entry(
                    plan.options->working_directory,
                    repository),
                L"repository command runs from the detected repository root");
            if (plan)
            {
                expect(plan.options->executable == fake_git.bash_executable &&
                        plan.options->arguments.size() == 4 &&
                        plan.options->arguments[0] == L"--login" &&
                        plan.options->arguments[1] == L"-c" &&
                        plan.options->arguments[2].starts_with(expected_script_prefix) &&
                        plan.options->arguments[3] == L"menu11" &&
                        (plan.options->creation_flags & CREATE_NEW_CONSOLE) != 0,
                    L"repository command uses bin\\bash.exe with a fixed script in a new console");
                expect(plan.options->arguments[2].find(repository.wstring()) == std::wstring::npos,
                    L"repository path is never interpolated into the shell script");
                expect(plan.options->arguments[2].find(L"read -r") != std::wstring::npos &&
                        !plan.options->wait_for_exit,
                    L"repository terminal stays readable while Runner exits immediately");
            }
        }

        const auto commit_plan = menu11::git::create_repository_launch_plan(
            fake_git,
            menu11::runner_command::commit,
            nested_directory);
        expect(commit_plan && commit_plan.options->executable == fake_git.gui_executable &&
                commit_plan.options->arguments == std::vector<std::wstring>{L"citool"} &&
                paths_refer_to_same_entry(commit_plan.options->working_directory, repository),
            L"Commit launches Git GUI citool from the repository root");
        auto git_without_command_bash = fake_git;
        git_without_command_bash.bash_executable.clear();
        expect(menu11::git::create_repository_launch_plan(
                git_without_command_bash,
                menu11::runner_command::status,
                nested_directory).error == menu11::git::repository_plan_error::executable_unavailable,
            L"repository commands require bin\\bash.exe rather than git-bash.exe");
        expect(menu11::git::create_repository_launch_plan(
                fake_git,
                menu11::runner_command::status,
                selected_file).error == menu11::git::repository_plan_error::invalid_selection,
            L"repository commands reject a file selection");
        expect(menu11::git::create_repository_launch_plan(
                fake_git,
                menu11::runner_command::status,
                non_repository).error == menu11::git::repository_plan_error::invalid_selection,
            L"repository commands reject a non-repository folder");
        const std::vector<std::filesystem::path> selected_files{selected_file, second_selected_file};
        const auto add_plan = menu11::git::create_file_launch_plan(
            fake_git,
            menu11::runner_command::file_add,
            selected_files);
        expect(add_plan && paths_refer_to_same_entry(
                add_plan.options->working_directory,
                repository),
            L"Add supports multiple files from one repository");
        if (add_plan)
        {
            expect(add_plan.options->arguments.size() == 6 &&
                    add_plan.options->arguments[0] == L"--login" &&
                    add_plan.options->arguments[1] == L"-c" &&
                    add_plan.options->arguments[2].starts_with(L"git add --") &&
                    add_plan.options->arguments[3] == L"menu11" &&
                    paths_refer_to_same_entry(add_plan.options->arguments[4], selected_file) &&
                    paths_refer_to_same_entry(add_plan.options->arguments[5], second_selected_file) &&
                    (add_plan.options->creation_flags & CREATE_NEW_CONSOLE) != 0,
                L"file paths are separate Bash positional arguments");
            expect(add_plan.options->arguments[2].find(selected_file.wstring()) == std::wstring::npos &&
                    add_plan.options->arguments[2].find(second_selected_file.wstring()) == std::wstring::npos,
                L"file paths are never interpolated into the shell script");
        }

        const auto restore_plan = menu11::git::create_file_launch_plan(
            fake_git,
            menu11::runner_command::file_restore,
            selected_files);
        expect(restore_plan && restore_plan.options->arguments.size() == 6 &&
                restore_plan.options->arguments[2].starts_with(L"git restore --"),
            L"Restore supports multiple files with fixed safe arguments");
        expect(menu11::git::create_file_launch_plan(
                fake_git,
                menu11::runner_command::file_diff,
                selected_files).error == menu11::git::file_plan_error::invalid_selection,
            L"single-file commands reject multi-selection");

        const std::vector<std::pair<menu11::runner_command, std::wstring_view>> single_file_commands{
            {menu11::runner_command::file_diff, L"git diff --"},
            {menu11::runner_command::file_log, L"git log --follow --"},
            {menu11::runner_command::file_blame, L"git blame --"},
        };
        const std::vector<std::filesystem::path> one_file{selected_file};
        for (const auto& [command, expected_script_prefix] : single_file_commands)
        {
            const auto plan = menu11::git::create_file_launch_plan(fake_git, command, one_file);
            expect(plan && plan.options->arguments.size() == 5 &&
                    plan.options->arguments[2].starts_with(expected_script_prefix) &&
                    paths_refer_to_same_entry(plan.options->arguments[4], selected_file),
                L"single-file Git command uses one positional file argument");
        }

        const auto clone_plan = menu11::git::create_directory_launch_plan(
            fake_git,
            menu11::runner_command::clone,
            non_repository);
        expect(clone_plan &&
                paths_refer_to_same_entry(clone_plan.options->working_directory, non_repository) &&
                clone_plan.options->arguments.size() == 4 &&
                clone_plan.options->arguments[0] == L"--login" &&
                clone_plan.options->arguments[2].find(L"git clone -- \"$menu11_url\"") != std::wstring::npos,
            L"Clone uses a fixed interactive script in the selected non-repository directory");
        if (clone_plan)
        {
            expect(clone_plan.options->arguments[2].find(non_repository.wstring()) == std::wstring::npos,
                L"Clone never interpolates the selected directory into its shell script");
        }

        const auto init_plan = menu11::git::create_directory_launch_plan(
            fake_git,
            menu11::runner_command::init,
            non_repository);
        expect(init_plan && init_plan.options->arguments[2].starts_with(L"git init;") &&
                init_plan.options->arguments[3] == L"menu11" &&
                (init_plan.options->creation_flags & CREATE_NEW_CONSOLE) != 0,
            L"Init runs in a selected non-repository directory");
        expect(menu11::git::create_directory_launch_plan(
                fake_git,
                menu11::runner_command::init,
                nested_directory).error == menu11::git::directory_plan_error::invalid_selection,
            L"Init is rejected inside an existing repository");

        test_shell_extension(
            std::filesystem::path(module_path.data()).parent_path() / L"Menu11.Shell.dll");
    }

    return failures == 0 ? 0 : 1;
}

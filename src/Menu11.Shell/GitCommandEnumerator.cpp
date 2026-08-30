#include "GitCommandEnumerator.h"

#include "GitChildCommand.h"

#include <windows.h>

#include <new>
#include <utility>

namespace menu11::shell
{
    git_command_enumerator::git_command_enumerator(
        std::vector<const git_command_definition*> definitions,
        IUnknown* site,
        const std::size_t index) noexcept
        : definitions_(std::move(definitions)),
          site_(site),
          index_(index < definitions_.size() ? index : definitions_.size())
    {
    }

    HRESULT git_command_enumerator::QueryInterface(REFIID interface_id, void** object)
    {
        if (object == nullptr)
        {
            return E_POINTER;
        }
        *object = nullptr;

        if (interface_id == IID_IUnknown || interface_id == __uuidof(IEnumExplorerCommand))
        {
            *object = static_cast<IEnumExplorerCommand*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG git_command_enumerator::AddRef()
    {
        return add_reference();
    }

    ULONG git_command_enumerator::Release()
    {
        return release_reference();
    }

    HRESULT git_command_enumerator::Next(
        const ULONG count,
        IExplorerCommand** commands,
        ULONG* fetched)
    {
        if ((count != 0 && commands == nullptr) || (count > 1 && fetched == nullptr))
        {
            return E_POINTER;
        }
        if (fetched != nullptr)
        {
            *fetched = 0;
        }
        if (count == 0)
        {
            return S_OK;
        }

        for (ULONG output_index = 0; output_index < count; ++output_index)
        {
            commands[output_index] = nullptr;
        }

        const auto starting_index = index_;
        ULONG completed = 0;
        while (completed < count && index_ < definitions_.size())
        {
            void* object = nullptr;
            const HRESULT result = create_git_child_command(
                *definitions_[index_],
                site_.Get(),
                __uuidof(IExplorerCommand),
                &object);
            if (FAILED(result))
            {
                for (ULONG output_index = 0; output_index < completed; ++output_index)
                {
                    commands[output_index]->Release();
                    commands[output_index] = nullptr;
                }
                index_ = starting_index;
                return result;
            }

            commands[completed] = static_cast<IExplorerCommand*>(object);
            ++completed;
            ++index_;
        }

        if (fetched != nullptr)
        {
            *fetched = completed;
        }
        return completed == count ? S_OK : S_FALSE;
    }

    HRESULT git_command_enumerator::Skip(const ULONG count)
    {
        const auto remaining = definitions_.size() - index_;
        if (static_cast<std::size_t>(count) <= remaining)
        {
            index_ += count;
            return S_OK;
        }

        index_ = definitions_.size();
        return S_FALSE;
    }

    HRESULT git_command_enumerator::Reset()
    {
        index_ = 0;
        return S_OK;
    }

    HRESULT git_command_enumerator::Clone(IEnumExplorerCommand** enumerator)
    {
        if (enumerator == nullptr)
        {
            return E_POINTER;
        }
        *enumerator = nullptr;

        try
        {
            auto* clone = new (std::nothrow) git_command_enumerator(
                definitions_,
                site_.Get(),
                index_);
            if (clone == nullptr)
            {
                return E_OUTOFMEMORY;
            }
            *enumerator = clone;
            return S_OK;
        }
        catch (...)
        {
            return E_OUTOFMEMORY;
        }
    }

    HRESULT create_git_command_enumerator(
        std::vector<const git_command_definition*> definitions,
        IUnknown* site,
        IEnumExplorerCommand** enumerator) noexcept
    {
        if (enumerator == nullptr)
        {
            return E_POINTER;
        }
        *enumerator = nullptr;

        auto* result = new (std::nothrow) git_command_enumerator(
            std::move(definitions),
            site);
        if (result == nullptr)
        {
            return E_OUTOFMEMORY;
        }
        *enumerator = result;
        return S_OK;
    }
}

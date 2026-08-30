#include "Menu11.Shared/RepositoryDetector.h"

#include <windows.h>

#include <vector>

namespace menu11
{
    namespace
    {
        constexpr std::size_t maximum_ancestor_checks = 256;

        [[nodiscard]] std::filesystem::path normalized_full_path(
            const std::filesystem::path& path) noexcept
        {
            if (path.empty())
            {
                return {};
            }

            const DWORD required = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
            if (required == 0)
            {
                return path.lexically_normal();
            }

            std::vector<wchar_t> buffer(static_cast<std::size_t>(required) + 1, L'\0');
            const DWORD written = GetFullPathNameW(
                path.c_str(),
                static_cast<DWORD>(buffer.size()),
                buffer.data(),
                nullptr);
            if (written == 0 || written >= buffer.size())
            {
                return path.lexically_normal();
            }

            return std::filesystem::path(buffer.data()).lexically_normal();
        }

        [[nodiscard]] DWORD attributes(const std::filesystem::path& path) noexcept
        {
            return path.empty() ? INVALID_FILE_ATTRIBUTES : GetFileAttributesW(path.c_str());
        }

        [[nodiscard]] std::filesystem::path starting_directory(
            const std::filesystem::path& path,
            DWORD path_attributes) noexcept
        {
            return (path_attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? path : path.parent_path();
        }
    }

    std::optional<std::filesystem::path> find_repository_root(
        const std::filesystem::path& start_path) noexcept
    {
        try
        {
            auto normalized = normalized_full_path(start_path);
            const DWORD start_attributes = attributes(normalized);
            if (start_attributes == INVALID_FILE_ATTRIBUTES)
            {
                return std::nullopt;
            }

            auto current = starting_directory(normalized, start_attributes);
            for (std::size_t depth = 0; depth < maximum_ancestor_checks && !current.empty(); ++depth)
            {
                if (attributes(current / L".git") != INVALID_FILE_ATTRIBUTES)
                {
                    return current;
                }

                const auto parent = current.parent_path();
                if (parent.empty() || parent == current)
                {
                    break;
                }
                current = parent;
            }
        }
        catch (...)
        {
        }

        return std::nullopt;
    }

    repository_context inspect_repository_context(
        const std::filesystem::path& selected_path) noexcept
    {
        repository_context context;
        try
        {
            context.selected_path = normalized_full_path(selected_path);
            const DWORD selected_attributes = attributes(context.selected_path);
            if (selected_attributes == INVALID_FILE_ATTRIBUTES)
            {
                return context;
            }

            context.selection_exists = true;
            context.selection_is_file = (selected_attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
            context.working_directory = context.selection_is_file
                ? context.selected_path.parent_path()
                : context.selected_path;
            context.repository_root = find_repository_root(context.working_directory);
        }
        catch (...)
        {
            return repository_context{};
        }

        return context;
    }
}

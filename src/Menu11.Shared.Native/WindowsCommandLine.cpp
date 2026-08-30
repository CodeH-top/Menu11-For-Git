#include "Menu11.Shared/WindowsCommandLine.h"

namespace menu11
{
    std::wstring quote_windows_argument(const std::wstring_view argument)
    {
        std::wstring result;
        result.reserve(argument.size() + 2);
        result.push_back(L'"');

        std::size_t backslash_count = 0;
        for (const wchar_t character : argument)
        {
            if (character == L'\\')
            {
                ++backslash_count;
                continue;
            }

            if (character == L'"')
            {
                result.append((backslash_count * 2) + 1, L'\\');
                result.push_back(L'"');
                backslash_count = 0;
                continue;
            }

            result.append(backslash_count, L'\\');
            backslash_count = 0;
            result.push_back(character);
        }

        result.append(backslash_count * 2, L'\\');
        result.push_back(L'"');
        return result;
    }

    std::wstring build_windows_command_line(
        const std::filesystem::path& executable,
        const std::span<const std::wstring> arguments)
    {
        std::wstring command_line = quote_windows_argument(executable.wstring());
        for (const auto& argument : arguments)
        {
            command_line.push_back(L' ');
            command_line.append(quote_windows_argument(argument));
        }
        return command_line;
    }
}

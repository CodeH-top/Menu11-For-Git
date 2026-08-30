#pragma once

#include "Menu11.Shared/RunnerProtocol.h"

#include <windows.h>

#include <filesystem>
#include <span>

namespace menu11::shell
{
    [[nodiscard]] HRESULT launch_runner(
        runner_command command,
        std::span<const std::filesystem::path> selections) noexcept;
}

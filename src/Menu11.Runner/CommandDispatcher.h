#pragma once

#include "Menu11.Shared/RunnerProtocol.h"

#include <windows.h>

namespace menu11::runner
{
    enum class dispatch_error
    {
        none,
        menu_disabled,
        command_disabled,
        git_not_found,
        git_bash_not_found,
        bash_not_found,
        git_gui_not_found,
        invalid_selection,
        repository_required,
        file_selection_required,
        non_repository_directory_required,
        confirmation_required,
        launch_failed,
        unsupported_command,
    };

    struct dispatch_result
    {
        dispatch_error error = dispatch_error::none;
        DWORD win32_error = ERROR_SUCCESS;

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return error == dispatch_error::none;
        }
    };

    [[nodiscard]] dispatch_result dispatch(
        const runner_request& request,
        bool destructive_operation_confirmed = false) noexcept;
}

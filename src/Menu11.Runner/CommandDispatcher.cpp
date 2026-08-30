#include "CommandDispatcher.h"

#include "Menu11.Shared/GitBash.h"
#include "Menu11.Shared/GitDetection.h"
#include "Menu11.Shared/GitGui.h"
#include "Menu11.Shared/GitRepositoryCommands.h"
#include "Menu11.Shared/ProcessLauncher.h"
#include "Menu11.Shared/ProductInfo.h"
#include "Menu11.Shared/RegistrySettings.h"

namespace menu11::runner
{
    namespace
    {
        [[nodiscard]] std::optional<git_command> configured_command(
            const runner_command command) noexcept
        {
            switch (command)
            {
            case runner_command::status:
                return git_command::status;
            case runner_command::pull:
                return git_command::pull;
            case runner_command::fetch:
                return git_command::fetch;
            case runner_command::push:
                return git_command::push;
            case runner_command::commit:
                return git_command::commit;
            case runner_command::repository_log:
                return git_command::repository_log;
            case runner_command::branch:
                return git_command::branch;
            case runner_command::stash:
                return git_command::stash;
            case runner_command::file_add:
                return git_command::file_add;
            case runner_command::file_diff:
                return git_command::file_diff;
            case runner_command::file_log:
                return git_command::file_log;
            case runner_command::file_blame:
                return git_command::file_blame;
            case runner_command::file_restore:
                return git_command::file_restore;
            case runner_command::clone:
                return git_command::clone;
            case runner_command::init:
                return git_command::init;
            default:
                return std::nullopt;
            }
        }

        [[nodiscard]] bool is_repository_command(const runner_command command) noexcept
        {
            return command == runner_command::status || command == runner_command::pull ||
                command == runner_command::fetch || command == runner_command::push ||
                command == runner_command::commit || command == runner_command::repository_log ||
                command == runner_command::branch || command == runner_command::stash;
        }

        [[nodiscard]] bool is_file_command(const runner_command command) noexcept
        {
            return command == runner_command::file_add || command == runner_command::file_diff ||
                command == runner_command::file_log || command == runner_command::file_blame ||
                command == runner_command::file_restore;
        }

        [[nodiscard]] bool is_directory_command(const runner_command command) noexcept
        {
            return command == runner_command::clone || command == runner_command::init;
        }
    }

    dispatch_result dispatch(
        const runner_request& request,
        const bool destructive_operation_confirmed) noexcept
    {
        try
        {
            const bool is_general_command = request.command == runner_command::git_bash ||
                request.command == runner_command::git_gui;
            const bool is_settings_command = request.command == runner_command::settings;
            const auto command_setting = configured_command(request.command);
            if (!is_general_command && !is_settings_command && !command_setting)
            {
                return {.error = dispatch_error::unsupported_command};
            }

            const registry_settings_store settings_store;
            const auto settings = settings_store.load();
            if (!settings.enabled)
            {
                return {.error = dispatch_error::menu_disabled};
            }
            if ((request.command == runner_command::git_bash && !settings.show_git_bash) ||
                (request.command == runner_command::git_gui && !settings.show_git_gui) ||
                (is_settings_command && !settings.show_settings_command) ||
                (command_setting && !settings.is_command_enabled(*command_setting)))
            {
                return {.error = dispatch_error::command_disabled};
            }

            if (is_settings_command)
            {
                const auto directory = current_process_directory();
                if (directory.empty())
                {
                    return {
                        .error = dispatch_error::launch_failed,
                        .win32_error = ERROR_PATH_NOT_FOUND,
                    };
                }

                process_launch_options options{
                    .executable = directory / product::app_executable,
                    .working_directory = directory,
                    .show_window = SW_SHOWNORMAL,
                    .wait_for_exit = false,
                };
                const auto result = launch_process(options);
                if (!result.started())
                {
                    return {
                        .error = dispatch_error::launch_failed,
                        .win32_error = result.error,
                    };
                }
                return {};
            }

            const auto git_installation = git::detect_git_for_windows(settings.git_path);
            if (!git_installation)
            {
                return {.error = dispatch_error::git_not_found};
            }
            std::optional<process_launch_options> launch_options;
            if (request.command == runner_command::git_bash)
            {
                const auto plan = git::create_bash_launch_plan(*git_installation, request.selections.front());
                if (!plan)
                {
                    return {
                        .error = plan.error == git::bash_plan_error::executable_unavailable
                            ? dispatch_error::git_bash_not_found
                            : dispatch_error::invalid_selection,
                    };
                }
                launch_options = std::move(plan.options);
            }
            else if (request.command == runner_command::git_gui)
            {
                const auto plan = git::create_gui_launch_plan(*git_installation, request.selections.front());
                if (!plan)
                {
                    return {
                        .error = plan.error == git::gui_plan_error::executable_unavailable
                            ? dispatch_error::git_gui_not_found
                            : dispatch_error::invalid_selection,
                    };
                }
                launch_options = std::move(plan.options);
            }
            else if (is_repository_command(request.command))
            {
                if (request.selections.size() != 1)
                {
                    return {.error = dispatch_error::invalid_selection};
                }

                const auto plan = git::create_repository_launch_plan(
                    *git_installation,
                    request.command,
                    request.selections.front());
                if (!plan)
                {
                    if (plan.error == git::repository_plan_error::invalid_selection)
                    {
                        return {.error = dispatch_error::repository_required};
                    }
                    if (plan.error == git::repository_plan_error::unsupported_command)
                    {
                        return {.error = dispatch_error::unsupported_command};
                    }
                    return {
                        .error = request.command == runner_command::commit
                            ? dispatch_error::git_gui_not_found
                            : dispatch_error::bash_not_found,
                    };
                }
                launch_options = std::move(plan.options);
            }
            else if (is_file_command(request.command))
            {
                const auto plan = git::create_file_launch_plan(
                    *git_installation,
                    request.command,
                    request.selections);
                if (!plan)
                {
                    if (plan.error == git::file_plan_error::invalid_selection)
                    {
                        return {.error = dispatch_error::file_selection_required};
                    }
                    if (plan.error == git::file_plan_error::unsupported_command)
                    {
                        return {.error = dispatch_error::unsupported_command};
                    }
                    return {.error = dispatch_error::bash_not_found};
                }
                if (request.command == runner_command::file_restore &&
                    !destructive_operation_confirmed)
                {
                    return {.error = dispatch_error::confirmation_required};
                }
                launch_options = std::move(plan.options);
            }
            else if (is_directory_command(request.command))
            {
                if (request.selections.size() != 1)
                {
                    return {.error = dispatch_error::non_repository_directory_required};
                }
                const auto plan = git::create_directory_launch_plan(
                    *git_installation,
                    request.command,
                    request.selections.front());
                if (!plan)
                {
                    if (plan.error == git::directory_plan_error::invalid_selection)
                    {
                        return {.error = dispatch_error::non_repository_directory_required};
                    }
                    if (plan.error == git::directory_plan_error::unsupported_command)
                    {
                        return {.error = dispatch_error::unsupported_command};
                    }
                    return {.error = dispatch_error::bash_not_found};
                }
                launch_options = std::move(plan.options);
            }
            else
            {
                return {.error = dispatch_error::unsupported_command};
            }

            const auto launch_result = launch_process(*launch_options);
            if (!launch_result.started())
            {
                return {
                    .error = dispatch_error::launch_failed,
                    .win32_error = launch_result.error,
                };
            }

            return {};
        }
        catch (...)
        {
            return {.error = dispatch_error::launch_failed, .win32_error = ERROR_NOT_ENOUGH_MEMORY};
        }
    }
}

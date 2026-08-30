#pragma once

#include "Menu11.Shared/RunnerProtocol.h"
#include "Menu11.Shared/Settings.h"
#include "Menu11.Shared/Localization.h"

#include <guiddef.h>

#include <span>

namespace menu11::shell
{
    enum class command_context
    {
        repository,
        file,
        non_repository_directory,
        any,
    };

    struct git_command_definition
    {
        GUID canonical_name;
        localized_string title;
        runner_command runner;
        unsigned int icon_resource_id;
        command_context context;
        git_command setting;
        bool requires_single_selection = false;
        bool separator_before = false;
        bool is_settings = false;
    };

    [[nodiscard]] std::span<const git_command_definition> git_command_definitions() noexcept;
}

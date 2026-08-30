#include "GitCommandDefinition.h"
#include "Resource.h"

#include <array>

namespace menu11::shell
{
    namespace
    {
        constexpr GUID make_guid(
            const unsigned long data1,
            const unsigned short data2,
            const unsigned short data3,
            const std::array<unsigned char, 8> data4) noexcept
        {
            return {data1, data2, data3, {
                data4[0], data4[1], data4[2], data4[3],
                data4[4], data4[5], data4[6], data4[7],
            }};
        }

        constexpr std::array definitions{
            git_command_definition{
                make_guid(0x639BFEA7, 0x4A0A, 0x4FDF, {0xB3, 0x7F, 0xFD, 0xD0, 0x1B, 0xCC, 0x2D, 0x10}),
                localized_string::status, runner_command::status, IDI_MENU11_STATUS,
                command_context::repository, git_command::status},
            git_command_definition{
                make_guid(0x5059B1DE, 0xA43E, 0x43C0, {0xB2, 0x8E, 0xB8, 0x97, 0x65, 0x66, 0xD1, 0x86}),
                localized_string::pull, runner_command::pull, IDI_MENU11_PULL,
                command_context::repository, git_command::pull},
            git_command_definition{
                make_guid(0x8833F40C, 0xA8B3, 0x4AD1, {0x91, 0x8C, 0xD9, 0x7B, 0x5F, 0x1C, 0x38, 0x17}),
                localized_string::fetch, runner_command::fetch, IDI_MENU11_FETCH,
                command_context::repository, git_command::fetch},
            git_command_definition{
                make_guid(0x350A44A6, 0x8A7B, 0x4356, {0x95, 0x5B, 0xB0, 0x03, 0x69, 0x2F, 0xDF, 0xFE}),
                localized_string::push, runner_command::push, IDI_MENU11_PUSH,
                command_context::repository, git_command::push},
            git_command_definition{
                make_guid(0xF8B09943, 0x9CA9, 0x4B35, {0xA8, 0xF3, 0x6A, 0x37, 0xF7, 0xDF, 0xA9, 0x19}),
                localized_string::commit, runner_command::commit, IDI_MENU11_COMMIT,
                command_context::repository, git_command::commit},
            git_command_definition{
                make_guid(0x69423BF3, 0x21E8, 0x4B7C, {0x9B, 0x7A, 0xC1, 0x05, 0xE9, 0xFC, 0x97, 0xCD}),
                localized_string::log, runner_command::repository_log, IDI_MENU11_LOG,
                command_context::repository, git_command::repository_log},
            git_command_definition{
                make_guid(0x73D289C2, 0x458A, 0x4F1D, {0xA4, 0xE6, 0xA0, 0xBC, 0x44, 0x89, 0x11, 0xEE}),
                localized_string::branch, runner_command::branch, IDI_MENU11_BRANCH,
                command_context::repository, git_command::branch},
            git_command_definition{
                make_guid(0x7E506D5A, 0x7641, 0x48E5, {0x88, 0x4D, 0x99, 0x96, 0xCD, 0x38, 0x46, 0x9C}),
                localized_string::stash, runner_command::stash, IDI_MENU11_STASH,
                command_context::repository, git_command::stash},
            git_command_definition{
                make_guid(0xFBB13C2B, 0xF004, 0x4EB4, {0x8F, 0xBB, 0x04, 0x05, 0xCB, 0x57, 0x5C, 0x08}),
                localized_string::add, runner_command::file_add, IDI_MENU11_ADD,
                command_context::file, git_command::file_add, false, true},
            git_command_definition{
                make_guid(0xDD020621, 0x3C6E, 0x457F, {0x84, 0xA1, 0xF0, 0x8C, 0x3D, 0x66, 0xCE, 0x55}),
                localized_string::diff, runner_command::file_diff, IDI_MENU11_DIFF,
                command_context::file, git_command::file_diff, true},
            git_command_definition{
                make_guid(0xDA00F938, 0xD285, 0x4394, {0x88, 0x5B, 0xA2, 0x01, 0xF4, 0x6F, 0xBF, 0xFD}),
                localized_string::log, runner_command::file_log, IDI_MENU11_LOG,
                command_context::file, git_command::file_log, true},
            git_command_definition{
                make_guid(0x383CBECB, 0x8798, 0x4B87, {0x81, 0xD7, 0xEB, 0x97, 0x0D, 0xA5, 0xF5, 0x24}),
                localized_string::blame, runner_command::file_blame, IDI_MENU11_BLAME,
                command_context::file, git_command::file_blame, true},
            git_command_definition{
                make_guid(0x77A07D90, 0x6BBD, 0x4395, {0x96, 0x62, 0x07, 0x59, 0x60, 0x26, 0xC5, 0x0B}),
                localized_string::restore, runner_command::file_restore, IDI_MENU11_RESTORE,
                command_context::file, git_command::file_restore},
            git_command_definition{
                make_guid(0xE2692E48, 0xF78C, 0x4544, {0xA5, 0x74, 0x15, 0xFE, 0xCD, 0x67, 0x28, 0xA1}),
                localized_string::clone, runner_command::clone, IDI_MENU11_CLONE,
                command_context::non_repository_directory, git_command::clone, true, true},
            git_command_definition{
                make_guid(0x80B7F9A3, 0x3F6C, 0x47A7, {0x8A, 0x29, 0xB8, 0xDC, 0xCE, 0xFC, 0x0D, 0x66}),
                localized_string::init_repository, runner_command::init, IDI_MENU11_INIT,
                command_context::non_repository_directory, git_command::init, true},
            git_command_definition{
                make_guid(0xD2C05DA4, 0xF41D, 0x44B7, {0xB8, 0xE3, 0x8B, 0x47, 0x75, 0x4E, 0x9D, 0xF7}),
                localized_string::settings, runner_command::settings, IDI_MENU11_SETTINGS,
                command_context::any, git_command::status, false, true, true},
        };
    }

    std::span<const git_command_definition> git_command_definitions() noexcept
    {
        return definitions;
    }
}

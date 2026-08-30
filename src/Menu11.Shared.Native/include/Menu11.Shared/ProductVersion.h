#pragma once

#define MENU11_VERSION_MAJOR 0
#define MENU11_VERSION_MINOR 1
#define MENU11_VERSION_PATCH 1
#define MENU11_VERSION_BUILD 0
#define MENU11_VERSION_COMMA 0,1,1,0
#define MENU11_VERSION_STRING "0.1.1"
#define MENU11_VERSION_RESOURCE_STRING "0.1.1\0"

#ifndef RC_INVOKED
namespace menu11::product
{
    inline constexpr unsigned int version_major = MENU11_VERSION_MAJOR;
    inline constexpr unsigned int version_minor = MENU11_VERSION_MINOR;
    inline constexpr unsigned int version_patch = MENU11_VERSION_PATCH;
    inline constexpr unsigned int version_build = MENU11_VERSION_BUILD;
}
#endif

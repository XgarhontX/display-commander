#pragma once

// Version information for Display Commander addon

#ifndef DISPLAY_COMMANDER_VERSION_HPP
#define DISPLAY_COMMANDER_VERSION_HPP

#if defined(__has_include)
#if __has_include("version_config.hpp")
#include "version_config.hpp"
#endif
#endif

#ifndef DISPLAY_COMMANDER_VERSION_MAJOR
#define DISPLAY_COMMANDER_VERSION_MAJOR 0
#endif
#ifndef DISPLAY_COMMANDER_VERSION_MINOR
#define DISPLAY_COMMANDER_VERSION_MINOR 0
#endif
#ifndef DISPLAY_COMMANDER_VERSION_PATCH
#define DISPLAY_COMMANDER_VERSION_PATCH 0
#endif
#ifndef GIT_COMMIT_COUNT
#define GIT_COMMIT_COUNT 0
#endif

// String conversion macro (used for version string)
#define STRINGIFY(x)  STRINGIFY_(x)
#define STRINGIFY_(x) #x

// Version string major.minor.patch (derived from the numeric macros above)
#define DISPLAY_COMMANDER_VERSION_STRING_MAJOR_MINOR_PATCH \
    STRINGIFY(DISPLAY_COMMANDER_VERSION_MAJOR)             \
    "." STRINGIFY(DISPLAY_COMMANDER_VERSION_MINOR) "." STRINGIFY(DISPLAY_COMMANDER_VERSION_PATCH)

// Build number from git commit count (set by CMake)
#define DISPLAY_COMMANDER_VERSION_BUILD        GIT_COMMIT_COUNT
#define DISPLAY_COMMANDER_VERSION_BUILD_STRING STRINGIFY(GIT_COMMIT_COUNT)

// Version string (includes build number)
#define DISPLAY_COMMANDER_VERSION_STRING \
    DISPLAY_COMMANDER_VERSION_STRING_MAJOR_MINOR_PATCH "." DISPLAY_COMMANDER_VERSION_BUILD_STRING

// Full version info string (version string already includes git commit count from CMake)
#define DISPLAY_COMMANDER_FULL_VERSION "Display Commander v" DISPLAY_COMMANDER_VERSION_STRING

#endif  // DISPLAY_COMMANDER_VERSION_HPP

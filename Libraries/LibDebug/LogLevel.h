/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string_view>

#include <Common/Types.h>

#ifdef OA_BUILD_DEBUG
#    define OA_LOG_COMPILED_LEVEL 0
#elifdef OA_BUILD_RELWITHDEBINFO
#    define OA_LOG_COMPILED_LEVEL 1
#elifdef OA_BUILD_DISTRIBUTION
#    define OA_LOG_COMPILED_LEVEL 2
#endif

namespace Debug {

enum class LogLevel : u8 {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    Off
};

inline constexpr LogLevel COMPILED_MIN_LEVEL = static_cast<LogLevel>(OA_LOG_COMPILED_LEVEL);

constexpr auto is_compiled(LogLevel level) -> bool
{
    return level >= COMPILED_MIN_LEVEL;
}

constexpr auto to_string(LogLevel level) -> std::string_view
{
    switch (level) {
    case LogLevel::Trace:
        return "TRACE";
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    case LogLevel::Fatal:
        return "FATAL";
    case LogLevel::Off:
        return "OFF";
    }
    return "UNKNOWN";
}

}

/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>

#include <Common/Expected.h>
#include <Common/Types.h>
#include <LibDebug/Export.h>
#include <LibDebug/LogLevel.h>

namespace Debug {

class Logger final {
public:
    struct Configuration {
        std::optional<std::filesystem::path> file_path {};
        std::filesystem::path directory = "Logs";
        LogLevel console_level { LogLevel::Info };
        LogLevel file_level { LogLevel::Trace };
        bool write_latest = true;
    };

    constexpr explicit Logger(std::string_view component)
        : m_component(component)
    {
    }

    static DEBUG_API auto initialize() -> Common::Expected<void>;
    static DEBUG_API auto initialize(Configuration const& config) -> Common::Expected<void>;
    static DEBUG_API void shutdown();

    static DEBUG_API void set_console_level(LogLevel level);
    static DEBUG_API void set_file_level(LogLevel level);
    static DEBUG_API auto is_enabled(LogLevel level) -> bool;

    static DEBUG_API auto file_path() -> std::filesystem::path;

    template<typename... Args>
    void trace(std::format_string<Args...> format, Args&&... args) const
    {
        log(LogLevel::Trace, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(std::format_string<Args...> format, Args&&... args) const
    {
        log(LogLevel::Debug, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(std::format_string<Args...> format, Args&&... args) const
    {
        log(LogLevel::Info, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(std::format_string<Args...> format, Args&&... args) const
    {
        log(LogLevel::Warn, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(std::format_string<Args...> format, Args&&... args) const
    {
        log(LogLevel::Error, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void fatal(std::format_string<Args...> format, Args&&... args) const
    {
        log(LogLevel::Fatal, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log(LogLevel level, std::format_string<Args...> format, Args&&... args) const
    {
        if (!is_enabled(level)) {
            return;
        }
        dispatch(level, m_component, std::format(format, std::forward<Args>(args)...));
    }
private:
    static DEBUG_API void dispatch(LogLevel level, std::string_view component, std::string_view message);

    std::string_view m_component;
};

}

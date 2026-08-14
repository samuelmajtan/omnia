/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <concepts>
#include <expected>
#include <filesystem>
#include <format>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>

#include <Common/Types.h>
#include <LibDebug/LogLevel.h>
#include <LibDebug/Export.h>

namespace Debug {

template<typename... Args>
struct FormatString final {
    std::format_string<Args...> value;
    std::source_location location;

    template<typename T>
        requires std::convertible_to<T const&, std::string_view>
    consteval FormatString(T const& format, std::source_location const& source = std::source_location::current())
        : value(format)
        , location(source)
    {
    }
};

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

    static DEBUG_API auto initialize() -> std::expected<void, std::string>;
    static DEBUG_API auto initialize(Configuration const& config) -> std::expected<void, std::string>;
    static DEBUG_API void shutdown();

    static DEBUG_API void set_console_level(LogLevel level);
    static DEBUG_API void set_file_level(LogLevel level);
    static DEBUG_API auto is_enabled(LogLevel level) -> bool;

    static DEBUG_API auto file_path() -> std::filesystem::path;

    template<typename... Args>
    void trace(FormatString<std::type_identity_t<Args>...> format, Args&&... args) const
    {
        log(LogLevel::Trace, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(FormatString<std::type_identity_t<Args>...> format, Args&&... args) const
    {
        log(LogLevel::Debug, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(FormatString<std::type_identity_t<Args>...> format, Args&&... args) const
    {
        log(LogLevel::Info, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(FormatString<std::type_identity_t<Args>...> format, Args&&... args) const
    {
        log(LogLevel::Warn, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(FormatString<std::type_identity_t<Args>...> format, Args&&... args) const
    {
        log(LogLevel::Error, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void fatal(FormatString<std::type_identity_t<Args>...> format, Args&&... args) const
    {
        log(LogLevel::Fatal, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log(LogLevel level, FormatString<std::type_identity_t<Args>...> format, Args&&... args) const
    {
        if (!is_enabled(level)) {
            return;
        }
        dispatch(level, m_component, std::format(format.value, std::forward<Args>(args)...), format.location);
    }
private:
    static DEBUG_API void dispatch(LogLevel level, std::string_view component, std::string_view message, std::source_location const& location);

    std::string_view m_component;
};

}
/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <atomic>
#include <cstdio>
#include <fstream>
#include <mutex>

#include <Common/File.h>
#include <Common/Time.h>
#include <LibDebug/Logger.h>

namespace Debug {

namespace {

struct LoggerState final {
    std::mutex mutex;
    std::ofstream file;
    std::ofstream latest;
    std::filesystem::path path;
    std::atomic<LogLevel> console_level = LogLevel::Info;
    std::atomic<LogLevel> file_level = LogLevel::Trace;
    std::atomic<bool> file_open = false;
};

auto logger_state() -> LoggerState&
{
    static LoggerState state;
    return state;
}

void close_files(LoggerState& state)
{
    state.file_open.store(false, std::memory_order_relaxed);

    if (state.file.is_open()) {
        state.file.flush();
        state.file.close();
    }
    if (state.latest.is_open()) {
        state.latest.flush();
        state.latest.close();
    }
    state.path.clear();
}

}

auto Logger::initialize() -> std::expected<void, std::string>
{
    return initialize(Configuration {});
}

auto Logger::initialize(Configuration const& config) -> std::expected<void, std::string>
{
    auto& state = logger_state();
    std::lock_guard const lock(state.mutex);

    close_files(state);

    state.console_level.store(config.console_level, std::memory_order_relaxed);
    state.file_level.store(config.file_level, std::memory_order_relaxed);

    if (config.file_level == LogLevel::Off) {
        return {};
    }

    auto path = config.file_path.has_value()
        ? config.file_path.value()
        : config.directory / std::format("Omnia_{}.log", Time::format_filename(Time::now()));

    if (auto result = File::create_parent_dir(path); !result.has_value()) {
        return result;
    }

    state.file.open(path, std::ios::trunc);
    if (!state.file.is_open()) {
        return std::unexpected(std::format("Failed to open log file: {}", path.string()));
    }

    if (config.write_latest) {
        state.latest.open(path.parent_path() / "Latest.log", std::ios::trunc);
    }

    state.path = path;
    state.file_open.store(true, std::memory_order_relaxed);
    return {};
}

void Logger::shutdown()
{
    auto& state = logger_state();
    std::lock_guard const lock(state.mutex);

    close_files(state);
}

void Logger::set_console_level(LogLevel level)
{
    logger_state().console_level.store(level, std::memory_order_relaxed);
}

void Logger::set_file_level(LogLevel level)
{
    logger_state().file_level.store(level, std::memory_order_relaxed);
}

auto Logger::is_enabled(LogLevel level) -> bool
{
    if (level == LogLevel::Off) {
        return false;
    }

    auto& state = Debug::logger_state();
    if (level >= state.console_level.load(std::memory_order_relaxed)) {
        return true;
    }
    return state.file_open.load(std::memory_order_relaxed) && level >= state.file_level.load(std::memory_order_relaxed);
}

auto Logger::file_path() -> std::filesystem::path
{
    auto& state = Debug::logger_state();
    std::lock_guard const lock(state.mutex);

    return state.path;
}

void Logger::dispatch(LogLevel level, std::string_view component, std::string_view message, std::source_location const& location)
{
    auto const timestamp = Time::format_log(Time::now());

    std::string origin;
    if (level >= LogLevel::Error) {
        auto location_path = std::filesystem::path(location.file_name());
        origin = std::format(" ({}:{})", location_path.filename().string(), location.line());
    }

    auto const line = std::format("[{}] [{} - {}]: {}{}\n", timestamp, component, to_string(level), message, origin);

    auto const to_standard_error = level >= LogLevel::Warn;
    auto* stream = to_standard_error ? stderr : stdout;

    auto& state = Debug::logger_state();
    std::lock_guard const lock(state.mutex);

    if (level >= state.console_level.load(std::memory_order_relaxed)) {
        std::fwrite(line.data(), 1, line.size(), stream);

        if (to_standard_error) {
            std::fflush(stream);
        }
    }

    if (state.file_open.load(std::memory_order_relaxed) && level >= state.file_level.load(std::memory_order_relaxed)) {
        state.file.write(line.data(), static_cast<std::streamsize>(line.size()));
        if (state.latest.is_open()) {
            state.latest.write(line.data(), static_cast<std::streamsize>(line.size()));
        }

        if (level >= LogLevel::Info) {
            state.file.flush();
            if (state.latest.is_open()) {
                state.latest.flush();
            }
        }
    }
}

}

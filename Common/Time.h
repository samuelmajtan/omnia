/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <chrono>
#include <ctime>
#include <format>
#include <string>
#include <utility>

#include <Common/Platform.h>
#include <Common/Types.h>

namespace Time {

using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;
using SteadyClock = std::chrono::steady_clock;

struct Timestamp final {
    i32 year {};
    u32 month {};
    u32 day {};
    i32 hour {};
    i32 minute {};
    i64 second {};
    i64 millisecond {};
};

inline constexpr auto now() -> TimePoint
{
    return Clock::now();
}

inline auto to_local(TimePoint time_point) -> Timestamp
{
    auto const local = std::chrono::current_zone()->to_local(time_point);
    auto const day = std::chrono::floor<std::chrono::days>(local);
    auto const time = local - day;
    auto const ymd = std::chrono::year_month_day(day);
    auto const hours = std::chrono::duration_cast<std::chrono::hours>(time);
    auto const minutes = std::chrono::duration_cast<std::chrono::minutes>(time - hours);
    auto const seconds = std::chrono::duration_cast<std::chrono::seconds>(time - hours - minutes);
    auto const milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time - hours - minutes - seconds);

    return Timestamp {
        .year = i32(ymd.year()),
        .month = u32(ymd.month()),
        .day = u32(ymd.day()),
        .hour = hours.count(),
        .minute = minutes.count(),
        .second = seconds.count(),
        .millisecond = milliseconds.count()
    };
}

inline auto format_log(Timestamp const& timestamp) -> std::string
{
    return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}",
        timestamp.year, timestamp.month, timestamp.day,
        timestamp.hour, timestamp.minute, timestamp.second, timestamp.millisecond);
}

inline auto format_log(TimePoint time_point) -> std::string
{
    return format_log(to_local(time_point));
}

inline auto format_filename(Timestamp const& timestamp) -> std::string
{
    return std::format("{:04}-{:02}-{:02}_{:02}-{:02}-{:02}",
        timestamp.year, timestamp.month, timestamp.day,
        timestamp.hour, timestamp.minute, timestamp.second);
}

inline auto format_filename(TimePoint time_point) -> std::string
{
    return format_filename(to_local(time_point));
}

class Stopwatch final {
public:
    Stopwatch()
        : m_start(SteadyClock::now())
    {
    }

    void restart()
    {
        m_start = SteadyClock::now();
    }

    auto elapsed() const -> std::chrono::nanoseconds
    {
        return SteadyClock::now() - m_start;
    }

    auto elapsed_milliseconds() const -> f64
    {
        return std::chrono::duration<f64, std::milli>(elapsed()).count();
    }

    auto elapsed_seconds() const -> f64
    {
        return std::chrono::duration<f64>(elapsed()).count();
    }
private:
    SteadyClock::time_point m_start;
};

}

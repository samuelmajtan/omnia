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
    i32 month {};
    i32 day {};
    i32 hour {};
    i32 minute {};
    i32 second {};
    i32 millisecond {};
};

inline constexpr auto now() -> TimePoint
{
    return Clock::now();
}

inline auto to_local(TimePoint time_point) -> Timestamp
{
    std::time_t time = Clock::to_time_t(time_point);
    std::tm local_tm {};
#if defined(OA_OS_WINDOWS)
    localtime_s(&local_tm, &time);
#else
    localtime_r(&time, &local_tm);
#endif

    auto const milliseconds = static_cast<i32>(std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count() % 1000);

    return Timestamp {
        .year = local_tm.tm_year + 1900,
        .month = local_tm.tm_mon + 1,
        .day = local_tm.tm_mday,
        .hour = local_tm.tm_hour,
        .minute = local_tm.tm_min,
        .second = local_tm.tm_sec,
        .millisecond = milliseconds
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

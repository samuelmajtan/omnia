/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string>
#include <thread>

#include <gtest/gtest.h>

#include <Common/Time.h>

namespace {

constexpr i64 EPOCH_SECONDS = 1786710605;

auto fixed_point() -> Time::TimePoint
{
    return Time::TimePoint {} + std::chrono::seconds(EPOCH_SECONDS) + std::chrono::milliseconds(123);
}

}

TEST(Time, FormatsBrokenDownTimeForLogPrefix)
{
    Time::Timestamp const timestamp {
        .year = 2026, .month = 8, .day = 14, .hour = 12, .minute = 30, .second = 5, .millisecond = 123
    };

    EXPECT_EQ(Time::format_log(timestamp), "2026-08-14 12:30:05.123");
    EXPECT_EQ(Time::format_filename(timestamp), "2026-08-14_12-30-05");
}

TEST(Time, PadsEveryFieldToFixedWidth)
{
    Time::Timestamp const timestamp {
        .year = 7, .month = 1, .day = 2, .hour = 3, .minute = 4, .second = 5, .millisecond = 6
    };

    EXPECT_EQ(Time::format_log(timestamp), "0007-01-02 03:04:05.006");
    EXPECT_EQ(Time::format_filename(timestamp), "0007-01-02_03-04-05");
}

TEST(Time, LogPrefixHasAStableWidth)
{
    EXPECT_EQ(Time::format_log(Time::now()).size(), 23u);
}

TEST(Time, FilenameFormatIsPathSafe)
{
    auto const name = Time::format_filename(Time::now());

    EXPECT_EQ(name.find(':'), std::string::npos);
    EXPECT_EQ(name.find('/'), std::string::npos);
    EXPECT_EQ(name.find('\\'), std::string::npos);
    EXPECT_EQ(name.size(), 19u);
}

TEST(Time, FilenameFormatSortsChronologically)
{
    auto const earlier = Time::format_filename(fixed_point());
    auto const later = Time::format_filename(fixed_point() + std::chrono::hours(30));

    EXPECT_LT(earlier, later);
}

TEST(Time, ExtractsMillisecondsIndependentlyOfTheZone)
{
    EXPECT_EQ(Time::to_local(fixed_point()).millisecond, 123);
    EXPECT_EQ(Time::to_local(fixed_point()).second, 5);
}

TEST(Time, MillisecondsRollOverAtASecondBoundary)
{
    auto const before = Time::to_local(fixed_point() + std::chrono::milliseconds(876));
    auto const after = Time::to_local(fixed_point() + std::chrono::milliseconds(877));

    EXPECT_EQ(before.millisecond, 999);
    EXPECT_EQ(after.millisecond, 0);
    EXPECT_EQ((before.second + 1) % 60, after.second);
}

TEST(Time, AdvancesTheCalendarAcrossADay)
{
    auto const start = Time::to_local(fixed_point());
    auto const next = Time::to_local(fixed_point() + std::chrono::hours(24));

    EXPECT_EQ(start.hour, next.hour);
    EXPECT_EQ(start.minute, next.minute);
    EXPECT_NE(start.day, next.day);
}

TEST(Time, StopwatchMeasuresForwardAndRestarts)
{
    Time::Stopwatch stopwatch;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    auto const elapsed = stopwatch.elapsed_milliseconds();
    EXPECT_GE(elapsed, 10.0);

    stopwatch.restart();
    EXPECT_LT(stopwatch.elapsed_milliseconds(), elapsed);
}

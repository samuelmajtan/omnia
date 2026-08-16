/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <algorithm>
#include <filesystem>
#include <format>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <Common/File.h>
#include <Common/UUID.h>
#include <LibDebug/Logger.h>

namespace {

constexpr Debug::Logger Logger { "TestComponent" };

class LoggerTest : public testing::Test {
protected:
    void SetUp() override
    {
        m_directory = std::filesystem::temp_directory_path() / std::format("omnia-log-{}", Common::UUID::generate().to_string());
    }

    void TearDown() override
    {
        Debug::Logger::shutdown();

        std::error_code error;
        std::filesystem::remove_all(m_directory, error);
    }

    auto configuration() const -> Debug::Logger::Configuration
    {
        return Debug::Logger::Configuration {
            .directory = m_directory,
            .console_level = Debug::LogLevel::Off,
            .file_level = Debug::LogLevel::Trace
        };
    }

    static auto read_lines(std::filesystem::path const& path) -> std::vector<std::string>
    {
        auto lines = File::read_lines(path);
        if (!lines.has_value()) {
            return {};
        }
        std::erase_if(lines.value(), [](std::string const& line) { return line.empty(); });
        return lines.value();
    }

    auto logged_lines() const -> std::vector<std::string>
    {
        return read_lines(m_directory / "Latest.log");
    }

    std::filesystem::path m_directory;
};

}

TEST_F(LoggerTest, WritesTheExpectedLineShape)
{
    ASSERT_TRUE(Debug::Logger::initialize(configuration()).has_value());

    Logger.info("Loaded {} assets in {}ms", 121, 84);
    Debug::Logger::shutdown();

    auto const lines = logged_lines();
    ASSERT_EQ(lines.size(), 1u);

    EXPECT_TRUE(lines[0].starts_with("["));
    constexpr std::string_view SUFFIX = "] [TestComponent] [INFO]: Loaded 121 assets in 84ms";
    EXPECT_EQ(lines[0].find(SUFFIX), 24u);
    EXPECT_EQ(lines[0].size(), 24u + SUFFIX.size());
}

TEST_F(LoggerTest, RecordsEveryLevelInOrder)
{
    ASSERT_TRUE(Debug::Logger::initialize(configuration()).has_value());

    Logger.trace("a");
    Logger.debug("b");
    Logger.info("c");
    Logger.warn("d");
    Logger.error("e");
    Logger.fatal("f");
    Debug::Logger::shutdown();

    auto const lines = logged_lines();
    ASSERT_EQ(lines.size(), 6u);

    EXPECT_NE(lines[0].find("[TRACE]: a"), std::string::npos);
    EXPECT_NE(lines[1].find("[DEBUG]: b"), std::string::npos);
    EXPECT_NE(lines[2].find("[INFO]: c"), std::string::npos);
    EXPECT_NE(lines[3].find("[WARN]: d"), std::string::npos);
    EXPECT_NE(lines[4].find("[ERROR]: e"), std::string::npos);
    EXPECT_NE(lines[5].find("[FATAL]: f"), std::string::npos);
}

TEST_F(LoggerTest, DoesNotAppendItsOwnCallSite)
{
    ASSERT_TRUE(Debug::Logger::initialize(configuration()).has_value());

    Logger.info("quiet");
    Logger.warn("also quiet");
    Logger.error("loud");
    Logger.fatal("loudest");
    Debug::Logger::shutdown();

    auto const lines = logged_lines();
    ASSERT_EQ(lines.size(), 4u);

    for (auto const& line : lines) {
        EXPECT_EQ(line.find("Logger.test.cpp:"), std::string::npos);
    }
}

TEST_F(LoggerTest, KeepsAnErrorsOwnOrigin)
{
    ASSERT_TRUE(Debug::Logger::initialize(configuration()).has_value());

    auto const failure = []() -> Common::Expected<void> { return OA_ERROR("The disk is on fire"); }();
    ASSERT_FALSE(failure.has_value());

    Logger.error("{}", failure.error());
    Debug::Logger::shutdown();

    auto const lines = logged_lines();
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_NE(lines[0].find("The disk is on fire (Logger.test.cpp:"), std::string::npos);
}

TEST_F(LoggerTest, DropsLinesBelowTheFileLevel)
{
    auto config = configuration();
    config.file_level = Debug::LogLevel::Warn;
    ASSERT_TRUE(Debug::Logger::initialize(config).has_value());

    Logger.trace("dropped");
    Logger.debug("dropped");
    Logger.info("dropped");
    Logger.warn("kept");
    Logger.error("kept");
    Debug::Logger::shutdown();

    auto const lines = logged_lines();
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_NE(lines[0].find("[WARN]: kept"), std::string::npos);
    EXPECT_NE(lines[1].find("[ERROR]: kept"), std::string::npos);
}

TEST_F(LoggerTest, SetFileLevelTakesEffectImmediately)
{
    ASSERT_TRUE(Debug::Logger::initialize(configuration()).has_value());

    Logger.trace("before");
    Debug::Logger::set_file_level(Debug::LogLevel::Error);
    Logger.trace("after");
    Logger.error("still here");
    Debug::Logger::shutdown();

    auto const lines = logged_lines();
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_NE(lines[0].find("before"), std::string::npos);
    EXPECT_NE(lines[1].find("still here"), std::string::npos);
}

TEST_F(LoggerTest, LatestMirrorsTheTimestampedFile)
{
    ASSERT_TRUE(Debug::Logger::initialize(configuration()).has_value());

    auto const path = Debug::Logger::file_path();
    EXPECT_NE(path.filename().string().find("Omnia_"), std::string::npos);

    Logger.info("mirrored {}", 1);
    Logger.warn("mirrored {}", 2);
    Debug::Logger::shutdown();

    auto const run = read_lines(path);
    ASSERT_EQ(run.size(), 2u);
    EXPECT_EQ(run, logged_lines());
}

TEST_F(LoggerTest, WritesToAnExplicitlyNamedFile)
{
    auto config = configuration();
    config.file_path = m_directory / "Nested" / "Custom.log";
    config.write_latest = false;
    ASSERT_TRUE(Debug::Logger::initialize(config).has_value());

    Logger.info("custom");
    Debug::Logger::shutdown();

    auto const lines = read_lines(m_directory / "Nested" / "Custom.log");
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_NE(lines[0].find("[INFO]: custom"), std::string::npos);
}

TEST_F(LoggerTest, ReinitializesAfterShutdown)
{
    ASSERT_TRUE(Debug::Logger::initialize(configuration()).has_value());
    Logger.info("first");
    Debug::Logger::shutdown();

    EXPECT_TRUE(Debug::Logger::file_path().empty());

    ASSERT_TRUE(Debug::Logger::initialize(configuration()).has_value());
    EXPECT_FALSE(Debug::Logger::file_path().empty());
    Logger.info("second");
    Debug::Logger::shutdown();

    auto const lines = logged_lines();
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_NE(lines[0].find("second"), std::string::npos);
}

TEST_F(LoggerTest, LogsFromManyThreadsWithoutTearingLines)
{
    ASSERT_TRUE(Debug::Logger::initialize(configuration()).has_value());

    constexpr u32 THREAD_COUNT = 8;
    constexpr u32 LINES_PER_THREAD = 500;

    {
        std::vector<std::jthread> writers;
        writers.reserve(THREAD_COUNT);
        for (u32 thread = 0; thread < THREAD_COUNT; thread++) {
            writers.emplace_back([thread] {
                for (u32 line = 0; line < LINES_PER_THREAD; line++) {
                    Logger.info("thread {} line {}", thread, line);
                }
            });
        }
    }
    Debug::Logger::shutdown();

    auto const lines = logged_lines();
    ASSERT_EQ(lines.size(), THREAD_COUNT * LINES_PER_THREAD);

    for (auto const& line : lines) {
        EXPECT_TRUE(line.starts_with("["));
        EXPECT_NE(line.find(" [TestComponent] [INFO]: thread "), std::string::npos);
    }
}

TEST_F(LoggerTest, LogsToConsoleWithoutInitialization)
{
    Debug::Logger::shutdown();

    EXPECT_TRUE(Debug::Logger::file_path().empty());
    EXPECT_FALSE(Debug::Logger::is_enabled(Debug::LogLevel::Off));

    Debug::Logger::set_console_level(Debug::LogLevel::Off);
    Logger.error("no sink configured");

    EXPECT_FALSE(std::filesystem::exists(m_directory));

    Debug::Logger::set_console_level(Debug::LogLevel::Info);
}

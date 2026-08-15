/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <filesystem>
#include <format>
#include <ostream>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Common {

class Error final {
public:
    explicit Error(std::string message, std::source_location origin = std::source_location::current())
        : m_message(std::move(message))
        , m_origin(origin)
    {
    }

    auto message() const -> std::string_view
    {
        return m_message;
    }

    auto origin() const -> std::source_location const&
    {
        return m_origin;
    }

    auto to_string() const -> std::string
    {
        return std::format("{} ({}:{})", m_message, std::filesystem::path(m_origin.file_name()).filename().string(), m_origin.line());
    }
private:
    std::string m_message;
    std::source_location m_origin;
};

inline auto operator<<(std::ostream& stream, Error const& error) -> std::ostream&
{
    return stream << error.to_string();
}

template<typename T>
using Expected = std::expected<T, Error>;

template<typename T, typename E>
constexpr inline auto unwrap(std::expected<T, E>&& expected) -> T
{
    if constexpr (!std::is_void_v<T>) {
        return std::move(expected).value();
    }
}

}

template<>
struct std::formatter<Common::Error> : std::formatter<std::string> {
    auto format(Common::Error const& error, std::format_context& context) const -> std::format_context::iterator
    {
        return std::formatter<std::string>::format(error.to_string(), context);
    }
};

#define OA_ERROR(...) \
    (::std::unexpected(::Common::Error(::std::format(__VA_ARGS__), ::std::source_location::current())))

#define TRY(expected)                                               \
    ({                                                              \
        auto result = (expected);                                   \
        if (!result.has_value()) {                                  \
            return ::std::unexpected(::std::move(result).error());  \
        }                                                           \
        ::Common::unwrap(::std::move(result));                      \
    })

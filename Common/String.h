/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string_view>

#include <Common/Types.h>

namespace String {

inline constexpr std::string_view WHITESPACE = " \t\n\v\f\r";

inline constexpr auto is_whitespace(char character) -> bool
{
    return WHITESPACE.find(character) != std::string_view::npos;
}

inline constexpr auto trimmed_left(std::string_view text) -> std::string_view
{
    while (!text.empty() && is_whitespace(text.front())) {
        text.remove_prefix(1);
    }
    return text;
}

inline constexpr auto trimmed_right(std::string_view text) -> std::string_view
{
    while (!text.empty() && is_whitespace(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

inline constexpr auto trimmed(std::string_view text) -> std::string_view
{
    return trimmed_right(trimmed_left(text));
}

}

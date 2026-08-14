/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <Common/Hash.h>
#include <Common/Random.h>
#include <Common/Types.h>

namespace Common {

class UUID final {
public:
    static constexpr std::size_t SIZE = 16;
    static constexpr std::size_t STRING_SIZE = 36;

    constexpr UUID() = default;

    constexpr UUID(u64 value)
    {
        for (std::size_t i = 0; i < sizeof(value); ++i) {
            m_data[i] = static_cast<std::byte>((value >> (i * 8)) & 0xFF);
        }
    }

    static auto generate() -> UUID
    {
        return generate(Random::shared());
    }

    static auto generate(Random& random) -> UUID
    {
        UUID uuid;
        auto& engine = random.engine();
        auto const high = engine();
        auto const low = engine();
        for (std::size_t i = 0; i < sizeof(high); ++i) {
            uuid.m_data[i] = static_cast<std::byte>((high >> ((7 - i) * 8)) & 0xFF);
            uuid.m_data[i + 8] = static_cast<std::byte>((low >> ((7 - i) * 8)) & 0xFF);
        }

        uuid.m_data[6] = (uuid.m_data[6] & std::byte { 0x0F }) | std::byte { 0x40 };
        uuid.m_data[8] = (uuid.m_data[8] & std::byte { 0x3F }) | std::byte { 0x80 };
        return uuid;
    }

    static constexpr auto from_string(std::string_view string) -> std::optional<UUID>
    {
        if (string.size() != STRING_SIZE) {
            return std::nullopt;
        }

        UUID uuid;
        std::size_t index = 0;
        for (std::size_t position = 0; position < STRING_SIZE;) {
            if (position == 8 || position == 13 || position == 18 || position == 23) {
                if (string[position] != '-') {
                    return std::nullopt;
                }
                ++position;
                continue;
            }

            auto const high = parse_hex_digit(string[position]);
            auto const low = parse_hex_digit(string[position + 1]);
            if (!high || !low) {
                return std::nullopt;
            }

            uuid.m_data[index++] = static_cast<std::byte>((high.value() << 4) | low.value());
            position += 2;
        }
        return uuid;
    }

    constexpr auto operator==(UUID const& other) const -> bool = default;

    constexpr auto version() const -> u8
    {
        return static_cast<u8>(m_data[6]) >> 4;
    }

    constexpr auto bytes() const -> std::span<std::byte const, SIZE>
    {
        return m_data;
    }

    auto hash() const -> std::size_t
    {
        return static_cast<std::size_t>(Hash::fnv1a(m_data));
    }

    constexpr auto to_string() const -> std::string
    {
        constexpr std::string_view digits = "0123456789abcdef";

        std::string string(STRING_SIZE, '-');
        std::size_t position = 0;
        for (std::size_t index = 0; index < SIZE; ++index) {
            if (index == 4 || index == 6 || index == 8 || index == 10) {
                ++position;
            }

            auto const value = static_cast<u8>(m_data[index]);
            string[position++] = digits[value >> 4];
            string[position++] = digits[value & 0x0F];
        }
        return string;
    }
private:
    static constexpr auto parse_hex_digit(char character) -> std::optional<u8>
    {
        if (character >= '0' && character <= '9') {
            return static_cast<u8>(character - '0');
        }
        if (character >= 'a' && character <= 'f') {
            return static_cast<u8>(character - 'a' + 10);
        }
        if (character >= 'A' && character <= 'F') {
            return static_cast<u8>(character - 'A' + 10);
        }
        return std::nullopt;
    }

    std::array<std::byte, SIZE> m_data {};
};

}

template<>
struct std::hash<Common::UUID> {
    auto operator()(Common::UUID const& uuid) const -> std::size_t
    {
        return uuid.hash();
    }
};

template<>
struct std::formatter<Common::UUID> : std::formatter<std::string> {
    auto format(Common::UUID const& uuid, std::format_context& context) const -> std::format_context::iterator
    {
        return std::formatter<std::string>::format(uuid.to_string(), context);
    }
};

/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <cstring>
#include <expected>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <Common/Expected.h>
#include <Common/Types.h>

namespace Binary {

template<typename T>
concept Serializable = std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>;

class ByteWriter final {
public:
    template<Serializable T>
    void write(T const& value)
    {
        auto const* bytes = reinterpret_cast<u8 const*>(&value);
        m_data.insert(m_data.end(), bytes, bytes + sizeof(T));
    }

    template<Serializable T>
    void write_vector(std::vector<T> const& values)
    {
        write(static_cast<u64>(values.size()));
        if (values.empty()) {
            return;
        }

        auto const* bytes = reinterpret_cast<u8 const*>(values.data());
        m_data.insert(m_data.end(), bytes, bytes + (values.size() * sizeof(T)));
    }

    void write_string(std::string_view value)
    {
        write(static_cast<u64>(value.size()));
        m_data.insert(m_data.end(), value.begin(), value.end());
    }

    void write_bytes(std::span<std::byte const> value)
    {
        auto const* bytes = reinterpret_cast<u8 const*>(value.data());
        m_data.insert(m_data.end(), bytes, bytes + value.size());
    }

    template<Serializable T>
    void write_optional(std::optional<T> const& value)
    {
        write(static_cast<u8>(value.has_value() ? 1 : 0));
        if (value.has_value()) {
            write(value.value());
        }
    }

    auto data() const -> std::vector<u8> const&
    {
        return m_data;
    }

    auto bytes() const -> std::span<std::byte const>
    {
        return std::as_bytes(std::span(m_data));
    }

    auto size() const -> u64
    {
        return m_data.size();
    }
private:
    std::vector<u8> m_data;
};

class ByteReader final {
public:
    explicit ByteReader(std::span<std::byte const> data)
        : m_data(data)
    {
    }

    template<Serializable T>
    auto read() -> Common::Expected<T>
    {
        if (sizeof(T) > remaining()) {
            return OA_ERROR("Truncated stream: wanted {} bytes, {} remaining", sizeof(T), remaining());
        }

        T value {};
        std::memcpy(&value, m_data.data() + m_offset, sizeof(T));
        m_offset += sizeof(T);
        return value;
    }

    template<Serializable T>
    auto read_vector() -> Common::Expected<std::vector<T>>
    {
        u64 count {};
        TRY_ASSIGN(count, read<u64>());

        if (count > remaining() / sizeof(T)) {
            return OA_ERROR("Truncated stream: wanted {} elements of {} bytes, {} remaining", count, sizeof(T), remaining());
        }

        std::vector<T> values(count);
        if (count > 0) {
            std::memcpy(values.data(), m_data.data() + m_offset, count * sizeof(T));
            m_offset += count * sizeof(T);
        }
        return values;
    }

    template<typename Enum>
    requires std::is_enum_v<Enum>
    auto read_enum(Enum lowest, Enum highest) -> Common::Expected<Enum>
    {
        u8 value {};
        TRY_ASSIGN(value, read<u8>());

        if (value < static_cast<u8>(lowest) || value > static_cast<u8>(highest)) {
            return OA_ERROR("Invalid enum value {} in stream, expected {} to {}", value, static_cast<u8>(lowest), static_cast<u8>(highest));
        }
        return static_cast<Enum>(value);
    }

    auto read_string() -> Common::Expected<std::string>
    {
        u64 length {};
        TRY_ASSIGN(length, read<u64>());

        if (length > remaining()) {
            return OA_ERROR("Truncated stream: wanted a {} byte string, {} remaining", length, remaining());
        }

        std::string value(reinterpret_cast<char const*>(m_data.data() + m_offset), length);
        m_offset += length;
        return value;
    }

    template<Serializable T>
    auto read_optional() -> Common::Expected<std::optional<T>>
    {
        u8 present {};
        TRY_ASSIGN(present, read<u8>());

        if (present == 0) {
            return std::optional<T> {};
        }

        T value {};
        TRY_ASSIGN(value, read<T>());
        return std::optional<T>(std::move(value));
    }

    auto remaining() const -> u64
    {
        return m_data.size() - m_offset;
    }

    auto is_exhausted() const -> bool
    {
        return remaining() == 0;
    }
private:
    std::span<std::byte const> m_data;
    u64 m_offset {};
};

}

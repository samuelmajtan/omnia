/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <span>
#include <string_view>

#include <Common/Types.h>

namespace Hash {

inline constexpr u64 FNV_OFFSET_BASIS   = 0xCBF29CE484222325ULL;
inline constexpr u64 FNV_PRIME          = 0x100000001B3ULL;

inline constexpr auto fnv1a(std::span<std::byte const> bytes, u64 seed = FNV_OFFSET_BASIS) -> u64
{
    auto hash = seed;
    for (auto byte : bytes) {
        hash ^= static_cast<u64>(byte);
        hash *= FNV_PRIME;
    }
    return hash;
}

inline constexpr auto fnv1a(std::string_view text, u64 seed = FNV_OFFSET_BASIS) -> u64
{
    auto hash = seed;
    for (auto character : text) {
        hash ^= static_cast<u64>(static_cast<u8>(character));
        hash *= FNV_PRIME;
    }
    return hash;
}

}

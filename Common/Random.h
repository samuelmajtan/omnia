/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <cassert>
#include <concepts>
#include <limits>
#include <random>

#include <Common/Types.h>

namespace Common {

class Random final {
public:
    using Engine = std::mt19937_64;

    Random()
    {
        reseed();
    }

    explicit Random(u64 seed)
    {
        reseed(seed);
    }

    static auto shared() -> Random&
    {
        static thread_local Random random;
        return random;
    }

    void reseed()
    {
        std::random_device device;
        std::seed_seq sequence { device(), device(), device(), device(), device(), device(), device(), device() };
        m_engine.seed(sequence);
    }

    void reseed(u64 seed)
    {
        m_engine.seed(seed);
    }

    auto next_u64() -> u64
    {
        return m_engine();
    }

    auto next_u32() -> u32
    {
        return static_cast<u32>(next_u64() >> 32);
    }

    auto next_f32() -> f32
    {
        return static_cast<f32>(next_u64() >> 40) * 0x1.0p-24f;
    }

    auto next_f64() -> f64
    {
        return static_cast<f64>(next_u64() >> 11) * 0x1.0p-53;
    }

    auto next_bool() -> bool
    {
        return (next_u64() >> 63) != 0;
    }

    template<std::integral T>
    auto range(T min, T max) -> T
    {
        assert(min <= max);

        auto const span = static_cast<u64>(max) - static_cast<u64>(min);
        if (span == std::numeric_limits<u64>::max()) {
            return static_cast<T>(next_u64());
        }

        auto const bound = span + 1;
        auto const threshold = (std::numeric_limits<u64>::max() - span) % bound;

        auto value = next_u64();
        while (value < threshold) {
            value = next_u64();
        }
        return static_cast<T>(static_cast<u64>(min) + (value % bound));
    }

    auto engine() -> Engine&
    {
        return m_engine;
    }
private:
    Engine m_engine;
};

}

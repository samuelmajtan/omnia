/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vector>

#include <Common/Types.h>

namespace Graphics {

enum class ShaderFormat : u8 {
    SPIRV = 0,
    DXIL,
    MetalIR,
    Count
};

enum class ShaderStage : u8 {
    Vertex = 0,
    Fragment,
    Count
};

struct ShaderVariant {
    ShaderFormat format;
    std::vector<u8> bytecode;
};

struct ShaderData {
    ShaderStage stage;
    std::vector<ShaderVariant> variants;
};

static constexpr auto operator|(ShaderStage lhs, ShaderStage rhs) -> ShaderStage
{
    return static_cast<ShaderStage>(static_cast<u8>(lhs) | static_cast<u8>(rhs));
}

static constexpr auto operator&(ShaderStage lhs, ShaderStage rhs) -> ShaderStage
{
    return static_cast<ShaderStage>(static_cast<u8>(lhs) & static_cast<u8>(rhs));
}

static constexpr auto any(ShaderStage stage) -> bool
{
    return static_cast<u8>(stage) != 0;
}

}

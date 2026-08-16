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

enum class ShaderStageMask : u8 {
    None = 0,
    Vertex = 1 << static_cast<u8>(ShaderStage::Vertex),
    Fragment = 1 << static_cast<u8>(ShaderStage::Fragment)
};

static constexpr auto to_mask(ShaderStage stage) -> ShaderStageMask
{
    return static_cast<ShaderStageMask>(1 << static_cast<u8>(stage));
}

static constexpr auto operator|(ShaderStageMask lhs, ShaderStageMask rhs) -> ShaderStageMask
{
    return static_cast<ShaderStageMask>(static_cast<u8>(lhs) | static_cast<u8>(rhs));
}

static constexpr auto operator&(ShaderStageMask lhs, ShaderStageMask rhs) -> ShaderStageMask
{
    return static_cast<ShaderStageMask>(static_cast<u8>(lhs) & static_cast<u8>(rhs));
}

static constexpr auto any(ShaderStageMask stages) -> bool
{
    return static_cast<u8>(stages) != 0;
}

}

/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Common/Noncopyable.h>
#include <Common/Types.h>
#include <LibGraphics/ShaderTypes.h>

namespace RHI {

enum class ResourceType : u8 {
    Sampler = 0,
    Texture,
    UniformBuffer,
};

class ResourceLayout {
    OA_MAKE_NONCOPYABLE(ResourceLayout);
    OA_MAKE_NONMOVABLE(ResourceLayout);

public:
    struct Binding {
        u32 binding;
        ResourceType type;
        Graphics::ShaderStageMask stage;
    };

    struct Configuration {
        std::vector<Binding> bindings;
    };

    virtual ~ResourceLayout() = default;
protected:
    ResourceLayout() = default;
};

}

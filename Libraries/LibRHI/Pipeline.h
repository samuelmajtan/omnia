/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <optional>
#include <vector>

#include <Common/Noncopyable.h>
#include <Common/Types.h>
#include <LibGraphics/ShaderTypes.h>
#include <LibRHI/Forward.h>

namespace RHI {

enum class CullMode : u8 {
    None = 0,
    Front,
    Back
};

enum class FrontFace : u8 {
    Clockwise = 0,
    CounterClockwise
};

enum class PolygonMode : u8 {
    Fill = 0,
    Line,
    Point
};

enum class CompareOp : u8 {
    Never = 0,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
};

enum class AttributeFormat : u8 {
    Float32 = 0,
    Float32Vec2,
    Float32Vec3,
    Float32Vec4,
    Uint32Vec4,
};

class Pipeline {
    OA_MAKE_NONCOPYABLE(Pipeline);
    OA_MAKE_NONMOVABLE(Pipeline);

public:
    struct Rasterization {
        CullMode cull_mode;
        FrontFace front_face;
        PolygonMode polygon_mode;
    };

    struct Depth {
        bool test_enable {};
        bool write_enable {};
        CompareOp compare_op = CompareOp::Never;
    };

    struct VertexAttribute {
        u32 location;
        u32 offset;
        AttributeFormat format;
    };

    struct VertexBinding {
        u32 stride;
        std::vector<VertexAttribute> attributes;
    };

    struct ColorBlendAttachment {
        bool blend_enable = false;
    };

    struct PushConstant {
        u32 size;
        u32 offset;
        Graphics::ShaderStageMask stage;
    };

    struct Configuration {
        Shader const* vertex_shader {};
        Shader const* fragment_shader {};
        Rasterization rasterization {};
        Depth depth {};
        RenderPass const* render_pass {};
        std::optional<VertexBinding> vertex_binding = std::nullopt;
        std::vector<ColorBlendAttachment> color_blend_attachments;
        std::vector<ResourceLayout const*> resource_layouts;
        std::vector<PushConstant> push_constants {};
    };

    virtual ~Pipeline() = default;
protected:
    Pipeline() = default;
};

}

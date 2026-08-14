/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <variant>

#include <Common/Noncopyable.h>
#include <Common/Types.h>
#include <LibMath/Math.h>
#include <LibRenderer/Export.h>

namespace Renderer {

class RENDERER_API Camera final {
    OA_MAKE_DEFAULT_CONSTRUCTIBLE(Camera);

public:
    struct PerspectiveConfiguration {
        f32 aspect_ratio {};
        f32 field_of_view_degrees {};
        f32 near_plane {};
        f32 far_plane {};
    };

    struct OrthographicConfiguration {
        f32 aspect_ratio {};
        f32 vertical_extent {};
        f32 near_plane {};
        f32 far_plane {};
    };

    using ProjectionConfiguration = std::variant<PerspectiveConfiguration, OrthographicConfiguration>;

    struct Configuration {
        ProjectionConfiguration projection { PerspectiveConfiguration {} };
        Math::Vec3f position {};
        Math::Quatf orientation {};
    };

    explicit Camera(Configuration const& config);

    auto position() const -> Math::Vec3f const&;
    auto orientation() const -> Math::Quatf const&;
    auto configuration() const -> ProjectionConfiguration const&;
    auto projection() const -> Math::Mat4f const&;
    auto view() const -> Math::Mat4f const&;

    auto forward() const -> Math::Vec3f;
    auto right() const -> Math::Vec3f;
    auto up() const -> Math::Vec3f;

    void rotate(f32 pitch_degrees, f32 yaw_degrees, f32 roll_degrees);
    void translate(Math::Vec3f const& translation);
    void set_position(Math::Vec3f const& position);
    void set_aspect_ratio(f32 aspect_ratio);
private:
    void update_projection();
    void update_view();

    Configuration m_config {};
    Math::Mat4f m_projection {};
    Math::Mat4f m_view {};
};

}

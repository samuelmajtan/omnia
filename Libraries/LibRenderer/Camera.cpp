/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibRenderer/Camera.h>

namespace Renderer {

Camera::Camera(Configuration const& config)
    : m_config(config)
{
    m_config.orientation = m_config.orientation.normalized();
    update_projection();
    update_view();
}

auto Camera::position() const -> Math::Vec3f const&
{
    return m_config.position;
}

auto Camera::orientation() const -> Math::Quatf const&
{
    return m_config.orientation;
}

auto Camera::configuration() const -> ProjectionConfiguration const&
{
    return m_config.projection;
}

auto Camera::projection() const -> Math::Mat4f const&
{
    return m_projection;
}

auto Camera::view() const -> Math::Mat4f const&
{
    return m_view;
}

auto Camera::forward() const -> Math::Vec3f
{
    return m_config.orientation * Math::Vec3f(0, 0, -1);
}

auto Camera::right() const -> Math::Vec3f
{
    return m_config.orientation * Math::Vec3f(1, 0, 0);
}

auto Camera::up() const -> Math::Vec3f
{
    return m_config.orientation * Math::Vec3f(0, 1, 0);
}

void Camera::rotate(f32 pitch_degrees, f32 yaw_degrees, f32 roll_degrees)
{
    auto const pitch_quat = Math::Quatf::from_axis_angle(Math::Vec3f { 1.0F, 0.0F, 0.0F }, DEG_TO_RAD(pitch_degrees));
    auto const yaw_quat = Math::Quatf::from_axis_angle(Math::Vec3f { 0.0F, 1.0F, 0.0F }, DEG_TO_RAD(yaw_degrees));
    auto const roll_quat = Math::Quatf::from_axis_angle(Math::Vec3f { 0.0F, 0.0F, 1.0F }, DEG_TO_RAD(roll_degrees));

    m_config.orientation = (yaw_quat * m_config.orientation * pitch_quat * roll_quat).normalized();
    update_view();
}

void Camera::translate(Math::Vec3f const& translation)
{
    m_config.position += translation;
    update_view();
}

void Camera::set_position(Math::Vec3f const& position)
{
    m_config.position = position;
    update_view();
}

void Camera::set_aspect_ratio(f32 aspect_ratio)
{
    std::visit([aspect_ratio](auto& projection) { projection.aspect_ratio = aspect_ratio; }, m_config.projection);
    update_projection();
}

void Camera::update_projection()
{
    m_projection = std::visit([](auto const& projection) {
        using Configuration = std::decay_t<decltype(projection)>;

        if constexpr (std::is_same_v<Configuration, PerspectiveConfiguration>) {
            return Math::Mat4f::perspective(DEG_TO_RAD(projection.field_of_view_degrees), projection.aspect_ratio, projection.near_plane, projection.far_plane);
        } else {
            auto const half_height = projection.vertical_extent / 2.0F;
            auto const half_width = half_height * projection.aspect_ratio;
            return Math::Mat4f::orthographic(-half_width, half_width, -half_height, half_height, projection.near_plane, projection.far_plane);
        }
    }, m_config.projection);
}

void Camera::update_view()
{
    m_view = Math::Mat4f::from_quaternion(m_config.orientation.conjugate()).translate(-m_config.position);
}

}

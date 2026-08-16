/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <array>
#include <cassert>
#include <cmath>
#include <tuple>

#include <Common/Types.h>
#include <LibMath/Quat.h>
#include <LibMath/Vec3.h>
#include <LibMath/Vec4.h>

namespace Math {

template<typename T = f32>
class Mat4 {
public:
    constexpr Mat4() = default;

    constexpr Mat4(T diagonal)
    {
        m_elements[0] = diagonal;
        m_elements[5] = diagonal;
        m_elements[10] = diagonal;
        m_elements[15] = diagonal;
    }

    constexpr Mat4(std::array<T, 16> elements)
        : m_elements(elements)
    {
    }

    constexpr auto data() -> T*
    {
        return m_elements.data();
    }

    constexpr auto at(std::size_t row, std::size_t col) -> T&
    {
        assert(row >= 0 && row < 4);
        assert(col >= 0 && col < 4);
        return m_elements[row + col * 4];
    }

    constexpr auto at(std::size_t row, std::size_t col) const -> T const&
    {
        assert(row >= 0 && row < 4);
        assert(col >= 0 && col < 4);
        return m_elements[row + col * 4];
    }

    constexpr auto operator[](size_t index) -> T&
    {
        assert(index >= 0 && index < m_elements.size());
        return m_elements[index];
    }

    constexpr auto operator[](size_t index) const -> T const&
    {
        assert(index >= 0 && index < m_elements.size());
        return m_elements[index];
    }

    constexpr auto operator*(Mat4<T> const& other) const -> Mat4<T>
    {
        Mat4<T> result {};
        for (size_t row = 0; row < 4; ++row) {
            for (size_t col = 0; col < 4; ++col) {
                for (size_t i = 0; i < 4; ++i) {
                    result.at(row, col) += at(row, i) * other.at(i, col);
                }
            }
        }
        return result;
    }

    constexpr auto operator*(Vec4<T> const& vec) const -> Vec4<T>
    {
        return Vec4<T> {
            at(0, 0) * vec.x + at(0, 1) * vec.y + at(0, 2) * vec.z + at(0, 3) * vec.w,
            at(1, 0) * vec.x + at(1, 1) * vec.y + at(1, 2) * vec.z + at(1, 3) * vec.w,
            at(2, 0) * vec.x + at(2, 1) * vec.y + at(2, 2) * vec.z + at(2, 3) * vec.w,
            at(3, 0) * vec.x + at(3, 1) * vec.y + at(3, 2) * vec.z + at(3, 3) * vec.w
        };
    }

    constexpr auto translate(Vec3<T> const& translation) const -> Mat4<T>
    {
        Mat4<T> result = *this;
        for (size_t row = 0; row < 4; ++row) {
            result.at(row, 3) += at(row, 0) * translation.x + at(row, 1) * translation.y + at(row, 2) * translation.z;
        }
        return result;
    }

    constexpr auto elements() const -> std::array<T, 16> const&
    {
        return m_elements;
    }

    static constexpr auto translation(T x, T y, T z) -> Mat4<T>
    {
        Mat4<T> result = identity();
        result.at(0, 3) = x;
        result.at(1, 3) = y;
        result.at(2, 3) = z;
        return result;
    }

    static constexpr auto translation(Vec3<T> const& translation) -> Mat4<T>
    {
        return Mat4<T>::translation(translation.x, translation.y, translation.z);
    }

    static constexpr auto rotation(T pitch, T yaw, T roll) -> Mat4<T>
    {
        auto const qx = Quat<T>::from_axis_angle(Vec3<T>{1, 0, 0}, pitch);
        auto const qy = Quat<T>::from_axis_angle(Vec3<T>{0, 1, 0}, yaw);
        auto const qz = Quat<T>::from_axis_angle(Vec3<T>{0, 0, 1}, roll);
        return from_quaternion(qz * qy * qx);
    }

    static constexpr auto rotation(Vec3<T> const& euler_angles) -> Mat4<T>
    {
        return rotation(euler_angles.x, euler_angles.y, euler_angles.z);
    }

    static constexpr auto scale(T x, T y, T z) -> Mat4<T>
    {
        Mat4<T> result = identity();
        result.at(0, 0) = x;
        result.at(1, 1) = y;
        result.at(2, 2) = z;
        return result;
    }

    static constexpr auto scale(Vec3<T> const& scale) -> Mat4<T>
    {
        return Mat4<T>::scale(scale.x, scale.y, scale.z);
    }

    static constexpr auto identity() -> Mat4<T>
    {
        return Mat4<T>(1);
    }

    static constexpr auto perspective(T fov, T aspect_ratio, T near_plane, T far_plane) -> Mat4<T>
    {
        auto const tan_half_fov = std::tan(fov / static_cast<T>(2));

        Mat4<T> result {};
        result.at(0, 0) = 1 / (aspect_ratio * tan_half_fov);
        result.at(1, 1) = -1 / tan_half_fov;
        result.at(2, 2) = -far_plane / (far_plane - near_plane);
        result.at(2, 3) = -(far_plane * near_plane) / (far_plane - near_plane);
        result.at(3, 2) = -1;
        return result;
    }

    static constexpr auto orthographic(T left, T right, T bottom, T top, T near_plane, T far_plane) -> Mat4<T>
    {
        Mat4<T> result {};
        result.at(0, 0) = 2 / (right - left);
        result.at(0, 3) = -(right + left) / (right - left);
        result.at(1, 1) = -2 / (top - bottom);
        result.at(1, 3) = (top + bottom) / (top - bottom);
        result.at(2, 2) = -1 / (far_plane - near_plane);
        result.at(2, 3) = -near_plane / (far_plane - near_plane);
        result.at(3, 3) = 1;
        return result;
    }

    static constexpr auto look_at(Vec3<T> const& eye, Vec3<T> const& center, Vec3<T> const& up) -> Mat4<T>
    {
        auto const f = (center - eye).normalized();
        auto const r = cross(f, up).normalized();
        auto const u = cross(r, f);

        Mat4<T> result {};
        result.at(0, 0) = r.x;
        result.at(0, 1) = r.y;
        result.at(0, 2) = r.z;
        result.at(0, 3) = -dot(r, eye);
        result.at(1, 0) = u.x;
        result.at(1, 1) = u.y;
        result.at(1, 2) = u.z;
        result.at(1, 3) = -dot(u, eye);
        result.at(2, 0) = -f.x;
        result.at(2, 1) = -f.y;
        result.at(2, 2) = -f.z;
        result.at(2, 3) = dot(f, eye);
        result.at(3, 3) = 1;
        return result;
    }

    static constexpr auto from_quaternion(Quat<T> const& quat) -> Mat4<T>
    {
        auto const xx = quat.x * quat.x;
        auto const yy = quat.y * quat.y;
        auto const zz = quat.z * quat.z;
        auto const xy = quat.x * quat.y;
        auto const xz = quat.x * quat.z;
        auto const yz = quat.y * quat.z;
        auto const wx = quat.w * quat.x;
        auto const wy = quat.w * quat.y;
        auto const wz = quat.w * quat.z;

        Mat4<T> result {};
        result.at(0, 0) = 1 - (2 * (yy + zz));
        result.at(0, 1) = 2 * (xy - wz);
        result.at(0, 2) = 2 * (xz + wy);
        result.at(1, 0) = 2 * (xy + wz);
        result.at(1, 1) = 1 - (2 * (xx + zz));
        result.at(1, 2) = 2 * (yz - wx);
        result.at(2, 0) = 2 * (xz - wy);
        result.at(2, 1) = 2 * (yz + wx);
        result.at(2, 2) = 1 - (2 * (xx + yy));
        result.at(3, 3) = 1;
        return result;
    }

    static constexpr auto from_trs(Vec3<T> const& translation, Quat<T> const& rotation, Vec3<T> const& scale) -> Mat4<T>
    {
        return Mat4<T>::translation(translation) * from_quaternion(rotation) * Mat4<T>::scale(scale);
    }

    constexpr auto to_quaternion() const -> Quat<T>
    {
        auto const trace = at(0, 0) + at(1, 1) + at(2, 2);
        if (trace > 0) {
            auto const s = std::sqrt(trace + 1) * 2;
            return Quat<T>((at(2, 1) - at(1, 2)) / s, (at(0, 2) - at(2, 0)) / s, (at(1, 0) - at(0, 1)) / s, s / 4);
        }
        if (at(0, 0) > at(1, 1) && at(0, 0) > at(2, 2)) {
            auto const s = std::sqrt(1 + at(0, 0) - at(1, 1) - at(2, 2)) * 2;
            return Quat<T>(s / 4, (at(0, 1) + at(1, 0)) / s, (at(0, 2) + at(2, 0)) / s, (at(2, 1) - at(1, 2)) / s);
        }
        if (at(1, 1) > at(2, 2)) {
            auto const s = std::sqrt(1 + at(1, 1) - at(0, 0) - at(2, 2)) * 2;
            return Quat<T>((at(0, 1) + at(1, 0)) / s, s / 4, (at(1, 2) + at(2, 1)) / s, (at(0, 2) - at(2, 0)) / s);
        }
        auto const s = std::sqrt(1 + at(2, 2) - at(0, 0) - at(1, 1)) * 2;
        return Quat<T>((at(0, 2) + at(2, 0)) / s, (at(1, 2) + at(2, 1)) / s, s / 4, (at(1, 0) - at(0, 1)) / s);
    }

    constexpr auto decompose() const -> std::tuple<Vec3<T>, Quat<T>, Vec3<T>>
    {
        Vec3<T> const x { at(0, 0), at(1, 0), at(2, 0) };
        Vec3<T> const y { at(0, 1), at(1, 1), at(2, 1) };
        Vec3<T> const z { at(0, 2), at(1, 2), at(2, 2) };

        Vec3<T> scale { x.length(), y.length(), z.length() };
        if (dot(cross(x, y), z) < 0) {
            scale.x = -scale.x;
        }

        Mat4<T> basis = identity();
        auto const unscale = [](Vec3<T> const& axis, T factor) {
            return factor == 0 ? Vec3<T> {} : axis / factor;
        };
        auto const nx = unscale(x, scale.x);
        auto const ny = unscale(y, scale.y);
        auto const nz = unscale(z, scale.z);

        basis.at(0, 0) = nx.x;
        basis.at(1, 0) = nx.y;
        basis.at(2, 0) = nx.z;
        basis.at(0, 1) = ny.x;
        basis.at(1, 1) = ny.y;
        basis.at(2, 1) = ny.z;
        basis.at(0, 2) = nz.x;
        basis.at(1, 2) = nz.y;
        basis.at(2, 2) = nz.z;

        return { Vec3<T> { at(0, 3), at(1, 3), at(2, 3) }, basis.to_quaternion().normalized(), scale };
    }
private:
    std::array<T, 16> m_elements {};
};

using Mat4f = Mat4<f32>;
using Mat4d = Mat4<f64>;
using Mat4i = Mat4<i32>;
using Mat4l = Mat4<i64>;
using Mat4u = Mat4<u32>;
using Mat4ul = Mat4<u64>;

}

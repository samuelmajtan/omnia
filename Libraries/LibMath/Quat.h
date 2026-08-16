/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <algorithm>
#include <cmath>

#include <Common/Types.h>
#include <LibMath/Vec3.h>

namespace Math {

template<typename T = f32>
class Quat {
public:
    T x {};
    T y {};
    T z {};
    T w {};

    constexpr Quat()
        : x(0)
        , y(0)
        , z(0)
        , w(1)
    {
    }

    constexpr Quat(T x, T y, T z, T w = 1)
        : x(x)
        , y(y)
        , z(z)
        , w(w)
    {
    }

    constexpr auto operator*(Vec3<T> const& vec) const -> Vec3<T>
    {
        auto normalized_quat = this->normalized();
        Vec3<T> const this_vec(normalized_quat.x, normalized_quat.y, normalized_quat.z);

        auto t = cross(this_vec, vec) * static_cast<T>(2);
        auto result = vec + (t * w) + cross(this_vec, t);
        return result;
    }

    constexpr auto operator*(Quat const& other) const -> Quat
    {
        return Quat(
            (w * other.x) + (x * other.w) + (y * other.z) - (z * other.y),
            (w * other.y) - (x * other.z) + (y * other.w) + (z * other.x),
            (w * other.z) + (x * other.y) - (y * other.x) + (z * other.w),
            (w * other.w) - (x * other.x) - (y * other.y) - (z * other.z));
    }

    constexpr auto operator*(T scalar) const -> Quat
    {
        return Quat(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    constexpr auto operator+(Quat const& other) const -> Quat
    {
        return Quat(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    constexpr auto operator-(Quat const& other) const -> Quat
    {
        return Quat(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    constexpr auto operator-() const -> Quat
    {
        return Quat(-x, -y, -z, -w);
    }

    constexpr auto operator==(Quat const& other) const -> bool
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    constexpr auto conjugate() const -> Quat
    {
        return Quat(-x, -y, -z, w);
    }

    constexpr auto inverse() const -> Quat
    {
        auto const length_squared = (x * x) + (y * y) + (z * z) + (w * w);
        if (length_squared == 0) {
            return Quat(0, 0, 0, 1);
        }

        auto const inverted_length_squared = 1.0F / length_squared;
        return Quat(-x * inverted_length_squared, -y * inverted_length_squared, -z * inverted_length_squared, w * inverted_length_squared);
    }

    constexpr auto length() const -> T
    {
        return std::sqrt((x * x) + (y * y) + (z * z) + (w * w));
    }

    constexpr void normalize()
    {
        auto const length = this->length();
        if (length == 0) {
            x = 0;
            y = 0;
            z = 0;
            w = 1;
            return;
        }

        auto const inverted_length = 1.0F / length;
        x *= inverted_length;
        y *= inverted_length;
        z *= inverted_length;
        w *= inverted_length;
    }

    constexpr auto normalized() const -> Quat
    {
        auto const length = this->length();
        if (length == 0) {
            return Quat(0, 0, 0, 1);
        }

        auto const inverted_length = 1.0F / length;
        return Quat(x * inverted_length, y * inverted_length, z * inverted_length, w * inverted_length);
    }

    static constexpr auto identity() -> Quat
    {
        return Quat(0, 0, 0, 1);
    }

    static constexpr auto from_axis_angle(Vec3<T> const& axis, f32 angle) -> Quat
    {
        auto const half_theta = angle / 2.0F;
        auto const sin_half_theta = std::sin(half_theta);
        auto const cos_half_theta = std::cos(half_theta);

        auto const length = axis.length();
        if (length == 0.0F) {
            return Quat(0, 0, 0, 1);
        }

        auto const inverted_length = 1.0F / length;
        return Quat(
            axis.x * inverted_length * sin_half_theta,
            axis.y * inverted_length * sin_half_theta,
            axis.z * inverted_length * sin_half_theta,
            cos_half_theta);
    }
};

template<typename T>
constexpr auto dot(Quat<T> const& a, Quat<T> const& b) -> T
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}

template<typename T>
constexpr auto nlerp(Quat<T> const& from, Quat<T> const& to, T t) -> Quat<T>
{
    auto const end = dot(from, to) < 0 ? -to : to;
    return (from + ((end - from) * t)).normalized();
}

template<typename T>
constexpr auto slerp(Quat<T> const& from, Quat<T> const& to, T t) -> Quat<T>
{
    auto cos_theta = std::clamp(dot(from, to), static_cast<T>(-1), static_cast<T>(1));
    auto end = to;
    if (cos_theta < 0) {
        end = -end;
        cos_theta = -cos_theta;
    }

    constexpr auto SLERP_LINEAR_THRESHOLD = static_cast<T>(0.9995);
    if (cos_theta > SLERP_LINEAR_THRESHOLD) {
        return nlerp(from, end, t);
    }

    auto const theta = std::acos(cos_theta);
    auto const sin_theta = std::sin(theta);
    auto const from_scale = std::sin((1 - t) * theta) / sin_theta;
    auto const end_scale = std::sin(t * theta) / sin_theta;
    return (from * from_scale) + (end * end_scale);
}

using Quatf = Quat<f32>;
using Quatd = Quat<f64>;
using Quati = Quat<i32>;
using Quatl = Quat<i64>;
using Quatu = Quat<u32>;
using Quatul = Quat<u64>;

}

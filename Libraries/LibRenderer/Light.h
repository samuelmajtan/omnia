/*
*  Copyright (c) 2026, the Omnia developers
*
* SPDX-License-Identifier: BSD-2-Clause
*/

#pragma once

#include <LibMath/Vec3.h>

namespace Renderer {

struct alignas(16) DirectionalLight {
    Math::Vec4f direction;
    Math::Vec4f color;
    Math::Mat4f space_matrix;
};

}
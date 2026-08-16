/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibDebug/Logger.h>

namespace RHI::Log {

inline constexpr Debug::Logger Device       { "LibRHI - Device" };
inline constexpr Debug::Logger Swapchain    { "LibRHI - Swapchain" };
inline constexpr Debug::Logger Pipeline     { "LibRHI - Pipeline" };
inline constexpr Debug::Logger Resources    { "LibRHI - Resources" };
inline constexpr Debug::Logger Validation   { "LibRHI - Validation" };

}

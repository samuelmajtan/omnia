/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibDebug/Logger.h>

namespace Renderer::Log {

inline constexpr Debug::Logger Resources    { "LibRenderer - Resources" };
inline constexpr Debug::Logger Deferred     { "LibRenderer - Deferred" };

}

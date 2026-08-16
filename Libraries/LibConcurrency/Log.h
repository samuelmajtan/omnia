/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibDebug/Logger.h>

namespace Concurrency::Log {

inline constexpr Debug::Logger Threads      { "LibConcurrency - Threads" };
inline constexpr Debug::Logger Scheduler    { "LibConcurrency - Scheduler" };

}

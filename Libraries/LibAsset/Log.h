/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibDebug/Logger.h>

namespace Asset::Log {

inline constexpr Debug::Logger Registry { "LibAsset - Registry" };
inline constexpr Debug::Logger Manager  { "LibAsset - Manager" };
inline constexpr Debug::Logger Model    { "LibAsset - Model" };
inline constexpr Debug::Logger Texture  { "LibAsset - Texture" };
inline constexpr Debug::Logger Shader   { "LibAsset - Shader" };

}

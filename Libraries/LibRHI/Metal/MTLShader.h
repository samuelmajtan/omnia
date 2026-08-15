/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <memory>
#include <string>

#include <Common/Expected.h>
#include <Common/Noncopyable.h>
#include <LibRHI/Shader.h>

namespace RHI {

class MTLShader final : public Shader {
    OA_MAKE_NONCOPYABLE(MTLShader);
    OA_MAKE_NONMOVABLE(MTLShader);

public:
    static auto create(Configuration const& config) -> Common::Expected<std::unique_ptr<MTLShader>>;

    ~MTLShader() override;

    auto config() const -> Configuration const& override;
private:
    MTLShader() = default;
private:
    Configuration m_config;
};

}

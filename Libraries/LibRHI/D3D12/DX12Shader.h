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

class DX12Shader final : public Shader {
    OA_MAKE_NONCOPYABLE(DX12Shader);
    OA_MAKE_NONMOVABLE(DX12Shader);

public:
    static auto create(Configuration const& config) -> Common::Expected<std::unique_ptr<DX12Shader>>;

    ~DX12Shader() override;

    auto config() const -> Configuration const& override;
private:
    DX12Shader() = default;
private:
    Configuration m_config;
};

}

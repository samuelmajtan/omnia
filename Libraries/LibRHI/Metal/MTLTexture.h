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
#include <LibRHI/Texture.h>

namespace RHI {

class MTLTexture final : public Texture {
    OA_MAKE_NONCOPYABLE(MTLTexture);
    OA_MAKE_NONMOVABLE(MTLTexture);

public:
    static auto create(Configuration const& config) -> Common::Expected<std::unique_ptr<MTLTexture>>;

    ~MTLTexture() override;

    auto config() const -> Configuration const& override;
private:
    MTLTexture() = default;
private:
    Configuration m_config {};
};

}

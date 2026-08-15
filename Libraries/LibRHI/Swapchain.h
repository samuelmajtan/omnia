/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <Common/Expected.h>
#include <Common/Noncopyable.h>
#include <Common/Types.h>
#include <LibRHI/Forward.h>
#include <LibRHI/Texture.h>

namespace RHI {

class Swapchain {
    OA_MAKE_NONCOPYABLE(Swapchain);
    OA_MAKE_NONMOVABLE(Swapchain);

public:
    struct Frame {
        CommandBuffer* cmd {};
        u32 image_index {};
        u32 frame_index {};
    };

    struct Configuration {
        u32 width {};
        u32 height {};
        i32 frames_in_flight = 2;
    };

    virtual ~Swapchain() = default;

    virtual auto width() const -> u32 = 0;
    virtual auto height() const -> u32 = 0;

    virtual auto format() const -> TextureFormat = 0;
    virtual auto textures() const -> std::vector<std::unique_ptr<Texture>> const& = 0;

    virtual auto is_dirty() const -> bool = 0;
    virtual auto recreate(Configuration const& config) -> Common::Expected<void> = 0;
    virtual void wait_idle() const = 0;
    virtual auto begin_frame() -> std::optional<Frame> = 0;
    virtual void end_frame(Frame const& frame) = 0;
protected:
    Swapchain() = default;
};

}

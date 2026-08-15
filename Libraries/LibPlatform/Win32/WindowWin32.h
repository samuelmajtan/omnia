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
#include <Common/Types.h>
#include <LibPlatform/Event.h>
#include <LibPlatform/Input.h>
#include <LibPlatform/Window.h>

struct HWND__;
struct HINSTANCE__;

using HWND = HWND__*;
using HINSTANCE = HINSTANCE__*;

namespace Platform {

class WindowWin32 final : public Window {
    OA_MAKE_NONCOPYABLE(WindowWin32);
    OA_MAKE_NONMOVABLE(WindowWin32);

public:
    ~WindowWin32() override;

    void poll_events() override;
    auto input() -> Input& override;
    auto event_dispatcher() -> EventDispatcher& override;

    auto is_running() const -> bool override;
    auto is_minimized() const -> bool override;

    auto title() const -> std::string const& override;
    auto width() const -> i32 override;
    auto height() const -> i32 override;

    auto PLATFORM_API instance() const -> HINSTANCE;
    auto PLATFORM_API handle() const -> HWND;

    static auto create(Configuration const& config) -> Common::Expected<std::unique_ptr<WindowWin32>>;
private:
    WindowWin32() = default;

    static auto window_procedure(HWND window_handle, u32 message, u64 first_param, i64 second_param) -> i64;
    auto handle_message(u32 message, u64 first_param, i64 second_param) -> bool;
private:
    Input m_input {};
    EventDispatcher m_dispatcher {};
    HWND m_handle {};
    HINSTANCE m_instance {};
    bool m_is_running = true;
    bool m_is_minimized = false;
    Configuration m_config {};
};

}

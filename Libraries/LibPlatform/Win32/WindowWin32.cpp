/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Windows.h>
#include <vector>
#include <windowsx.h>

#include <Common/Expected.h>
#include <LibPlatform/Event.h>
#include <LibPlatform/Log.h>
#include <LibPlatform/Win32/WindowWin32.h>

namespace Platform {

auto Window::create(Configuration const& config) -> Common::Expected<std::unique_ptr<Window>>
{
    return WindowWin32::create(config);
}

auto WindowWin32::create(Configuration const& config) -> Common::Expected<std::unique_ptr<WindowWin32>>
{
    std::unique_ptr<WindowWin32> window(new WindowWin32);

    window->m_instance = GetModuleHandle(nullptr);
    window->m_config = config;
    window->m_input = Input(&window->m_dispatcher);

    WNDCLASSEX const window_class {
        .cbSize = sizeof(WNDCLASSEX),
        .style = 0,
        .lpfnWndProc = window_procedure,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = window->m_instance,
        .hIcon = nullptr,
        .hCursor = LoadCursor(nullptr, IDC_ARROW),
        .hbrBackground = nullptr,
        .lpszMenuName = nullptr,
        .lpszClassName = "omnia_window",
        .hIconSm = nullptr
    };
    if (!RegisterClassEx(&window_class)) {
        return OA_ERROR("Failed to register window class");
    }

    DWORD constexpr style = WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX | WS_SYSMENU;
    RECT rect { 0, 0, window->m_config.width, window->m_config.height };
    AdjustWindowRect(&rect, style, FALSE);

    window->m_handle = CreateWindowEx(
        0,
        window_class.lpszClassName,
        window->m_config.title.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, window->m_instance, window.get());
    if (window->m_handle == nullptr) {
        return OA_ERROR("Failed to create window");
    }
    ShowWindow(window->m_handle, SW_SHOW);

    RAWINPUTDEVICE const raw_input_device {
        .usUsagePage = 0x01,
        .usUsage = 0x02,
        .dwFlags = 0,
        .hwndTarget = nullptr
    };
    if (RegisterRawInputDevices(&raw_input_device, 1, sizeof(raw_input_device)) == FALSE) {
        return OA_ERROR("Failed to register raw input device");
    }
    OA_LOG_DEBUG(Log::Window, "Raw mouse input registered");

    OA_LOG_INFO(Log::Window, "Window '{}' created at {}x{}", window->m_config.title, window->m_config.width, window->m_config.height);
    return window;
}

WindowWin32::~WindowWin32()
{
    OA_LOG_DEBUG(Log::Window, "Destroying window '{}'", m_config.title);

    DestroyWindow(m_handle);
    UnregisterClass("omnia_window", m_instance);
}

auto WindowWin32::window_procedure(HWND window_handle, u32 message, u64 first_param, i64 second_param) -> i64
{
    WindowWin32* window {};

    if (message == WM_NCCREATE) {
        auto* create_struct = reinterpret_cast<CREATESTRUCT*>(second_param);
        window = static_cast<WindowWin32*>(create_struct->lpCreateParams);
        SetWindowLongPtr(window_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<WindowWin32*>(GetWindowLongPtr(window_handle, GWLP_USERDATA));
    }

    if (window != nullptr && window->handle_message(message, first_param, second_param)) {
        return 0;
    }

    return DefWindowProc(window_handle, message, first_param, second_param);
}

auto WindowWin32::handle_message(u32 message, u64 first_param, i64 second_param) -> bool
{
    switch (message) {
    case WM_CLOSE:
        m_is_running = false;

        OA_LOG_INFO(Log::Window, "Window close requested");
        m_dispatcher.dispatch(WindowCloseEvent {});
        return true;
    case WM_DESTROY:
        OA_LOG_DEBUG(Log::Window, "Window destroyed");
        PostQuitMessage(0);
        return true;
    case WM_SIZE: {
        auto width = LOWORD(second_param);
        auto height = HIWORD(second_param);

        if (width == 0 || height == 0) {
            if (!m_is_minimized) {
                OA_LOG_DEBUG(Log::Window, "Window minimized");
            }
            m_is_minimized = true;
            return true;
        }

        if (m_is_minimized) {
            OA_LOG_DEBUG(Log::Window, "Window restored at {}x{}", width, height);
        }

        m_is_minimized = false;
        m_config.width = LOWORD(second_param);
        m_config.height = HIWORD(second_param);

        WindowResizeEvent event {
            .width = m_config.width,
            .height = m_config.height
        };
        m_dispatcher.dispatch(event);
        return true;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        auto const is_pressed = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
        auto const was_pressed = (second_param & (1 << 30)) != 0;

        m_input.handle_key(static_cast<Key>(first_param), is_pressed, was_pressed);
        return true;
    }
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP: {
        auto const is_pressed = (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN);

        auto button = MouseButton::Middle;
        switch (message) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            button = MouseButton::Left;
            break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            button = MouseButton::Right;
            break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            button = MouseButton::Middle;
            break;
        }

        auto const x = GET_X_LPARAM(second_param);
        auto const y = GET_Y_LPARAM(second_param);
        m_input.handle_mouse_button(button, is_pressed, { x, y });
        return true;
    }
    case WM_MOUSEMOVE: {
        auto const x = GET_X_LPARAM(second_param);
        auto const y = GET_Y_LPARAM(second_param);
        m_input.handle_mouse_move({ x, y });
        return true;
    }
    case WM_INPUT: {
        u32 data_size = 0;
        GetRawInputData(reinterpret_cast<HRAWINPUT>(second_param), RID_INPUT, nullptr, &data_size, sizeof(RAWINPUTHEADER));
        if (data_size > 0) {
            std::vector<std::byte> raw_buffer(data_size);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(second_param), RID_INPUT, raw_buffer.data(), &data_size, sizeof(RAWINPUTHEADER)) == data_size) {
                auto* raw_input = reinterpret_cast<RAWINPUT*>(raw_buffer.data());
                if (raw_input->header.dwType == RIM_TYPEMOUSE) {
                    auto const dx = static_cast<i32>(raw_input->data.mouse.lLastX);
                    auto const dy = static_cast<i32>(raw_input->data.mouse.lLastY);
                    m_input.handle_mouse_delta(dx, dy);
                }
            }
        }
    }
    default:
        return false;
    }
}

void WindowWin32::poll_events()
{
    MSG msg {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

auto WindowWin32::input() -> Input&
{
    return m_input;
}

auto WindowWin32::event_dispatcher() -> EventDispatcher&
{
    return m_dispatcher;
}

auto WindowWin32::is_minimized() const -> bool
{
    return m_is_minimized;
}

auto WindowWin32::is_running() const -> bool
{
    return m_is_running;
}

auto WindowWin32::title() const -> std::string const&
{
    return m_config.title;
}

auto WindowWin32::width() const -> i32
{
    return m_config.width;
}

auto WindowWin32::height() const -> i32
{
    return m_config.height;
}

auto WindowWin32::instance() const -> HINSTANCE
{
    return m_instance;
}

auto WindowWin32::handle() const -> HWND
{
    return m_handle;
}

}

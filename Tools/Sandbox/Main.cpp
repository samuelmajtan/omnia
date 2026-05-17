/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <print>
#include <ranges>

#include <Common/Expected.h>
#include <Common/Types.h>
#include <LibAsset/AssetManager.h>
#include <LibMath/Math.h>
#include <LibPlatform/Event.h>
#include <LibPlatform/Window.h>
#include <LibRHI/Device.h>
#include <LibRenderer/Camera.h>
#include <LibRenderer/DeferredRenderer.h>
#include <LibRenderer/Light.h>
#include <LibRenderer/Model.h>
#include <LibRenderer/ResourceManager.h>

class Sandbox final {
public:
    static auto create() -> std::expected<std::unique_ptr<Sandbox>, std::string>
    {
        std::unique_ptr<Sandbox> sandbox(new Sandbox);

        sandbox->m_asset_manager.load_loose_assets();

        Platform::Window::Configuration const window_config {
            .title = "Omnia Sandbox",
            .width = 800,
            .height = 600
        };
        TRY_ASSIGN(sandbox->m_window, Platform::Window::create(window_config));

        RHI::Device::Configuration const device_config {
            .api = RHI::Device::API::Vulkan,
            .enable_debug_layer = true,
            .window = sandbox->m_window.get(),
        };
        TRY_ASSIGN(sandbox->m_graphics_device, RHI::Device::create(device_config));
        TRY_ASSIGN(sandbox->m_resource_manager, Renderer::ResourceManager::create(&sandbox->m_asset_manager, sandbox->m_graphics_device.get()));

        RHI::Swapchain::Configuration const swapchain_config {
            .width = static_cast<u32>(window_config.width),
            .height = static_cast<u32>(window_config.height),
            .frames_in_flight = 2
        };
        TRY_ASSIGN(sandbox->m_swapchain, sandbox->m_graphics_device->create_swapchain(swapchain_config));

        Renderer::DeferredRenderer::Configuration const deferred_renderer_config {
            .render_target_width = swapchain_config.width,
            .render_target_height = swapchain_config.height,
            .render_target_format = sandbox->m_swapchain->format(),
            .device = sandbox->m_graphics_device.get(),
            .resource_manager = sandbox->m_resource_manager.get(),
        };
        TRY_ASSIGN(sandbox->m_deferred_renderer, Renderer::DeferredRenderer::create(deferred_renderer_config));

        Renderer::Camera::Configuration const camera_config {
            .projection_type = Renderer::ProjectionType::Perspective,
            .position = { 0.0F, 0.0F, 0.0F },
            .perspective = {
                .aspect_ratio = static_cast<f32>(swapchain_config.width) / static_cast<f32>(swapchain_config.height),
                .field_of_view_degrees = 90.0F,
                .near_plane = 0.1F,
                .far_plane = 100000.0F
            }
        };
        sandbox->m_camera = Renderer::Camera(camera_config);

        TRY_ASSIGN(sandbox->m_sponza, sandbox->m_resource_manager->load_model("Models/sponza/Sponza"));

        if (auto result = sandbox->create_swapchain_render_targets(); !result.has_value()) {
            return std::unexpected(std::move(result).error());
        }

        sandbox->m_window->event_dispatcher().register_listener<Platform::MouseDeltaEvent>(std::bind_front(&Sandbox::on_mouse_delta, sandbox.get()));
        sandbox->m_window->event_dispatcher().register_listener<Platform::WindowResizeEvent>(std::bind_front(&Sandbox::on_resize, sandbox.get()));
        return sandbox;
    }

    auto on_mouse_delta(Platform::MouseDeltaEvent const& event) -> bool
    {
        m_camera.rotate(static_cast<f32>(-event.dy) * 0.1F, static_cast<f32>(-event.dx) * 0.1F, 0.0F);
        return true;
    }

    auto on_resize([[maybe_unused]] Platform::WindowResizeEvent const& event) -> bool
    {
        m_camera.set_aspect_ratio(static_cast<f32>(m_window->width()) / static_cast<f32>(m_window->height()));
        m_was_window_resized = true;
        return true;
    }

    auto create_swapchain_render_targets() -> std::expected<void, std::string>
    {
        m_swapchain_render_targets.clear();

        auto const& swapchain_textures = m_swapchain->textures();
        m_swapchain_render_targets.resize(swapchain_textures.size());
        for (auto const& [index, texture] : std::views::enumerate(swapchain_textures)) {
            TRY_ASSIGN(m_swapchain_render_targets[index], m_deferred_renderer->create_output_render_target(texture.get()));
        }
        return {};
    }

    auto recreate_swapchain() -> bool
    {
        m_swapchain->wait_idle();
        RHI::Swapchain::Configuration const swapchain_config {
            .width = static_cast<u32>(m_window->width()),
            .height = static_cast<u32>(m_window->height()),
            .frames_in_flight = 2
        };

        if (auto result = m_swapchain->recreate(swapchain_config); !result.has_value()) {
            std::println(stderr, "{}", result.error());
            return false;
        }
        if (auto result = create_swapchain_render_targets(); !result.has_value()) {
            std::println(stderr, "{}", result.error());
            return false;
        }
        if (auto result = m_deferred_renderer->resize(swapchain_config.width, swapchain_config.height); !result.has_value()) {
            std::println(stderr, "{}", result.error());
            return false;
        }
        return true;
    }

    void run()
    {
        while (m_window->is_running()) {
            m_window->poll_events();

            if (m_window->is_minimized()) {
                continue;
            }

            if (m_was_window_resized || m_swapchain->is_dirty()) {
                m_was_window_resized = false;
                if (!recreate_swapchain()) {
                    break;
                }
                continue;
            }

            {
                using namespace Platform;
                Math::Vec3f movement {};
                movement += m_camera.forward() * (m_window->input().is_key_down(Key::W) ? 1.0F : 0.0F);
                movement -= m_camera.forward() * (m_window->input().is_key_down(Key::S) ? 1.0F : 0.0F);
                movement -= m_camera.right() * (m_window->input().is_key_down(Key::A) ? 1.0F : 0.0F);
                movement += m_camera.right() * (m_window->input().is_key_down(Key::D) ? 1.0F : 0.0F);
                movement.normalize();
                m_camera.translate(movement * 0.075F);
            }

            auto frame = m_swapchain->begin_frame();
            if (!frame.has_value()) {
                continue;
            }
            auto [cmd, image_index, frame_index] = frame.value();

            // TODO: Don't create this every frame, just update the model matrix
            std::vector<Renderer::RenderItem> render_items;
            render_items.reserve(m_sponza->sub_meshes().size());
            for (auto const& sub_mesh : m_sponza->sub_meshes()) {
                auto const& materials = m_sponza->materials();
                render_items.push_back({
                    .vertex_buffer = sub_mesh.vertex_buffer(),
                    .index_buffer = sub_mesh.index_buffer(),
                    .material_resource_set = materials[sub_mesh.material_index()].resource_set(),
                    .index_count = sub_mesh.index_count(),
                    .model_matrix = Math::Mat4f::scale(0.05F, 0.05F, 0.05F)
                });
            }

            auto light_direction = Math::Vec3f(0.0F, -1.0F, 0.5F).normalized();
            auto light_position = -light_direction * 1000.0F;
            auto light_projection = Math::Mat4f::orthographic(-600.0F, 600.0F, -500.0F, 500.0F, 0.001F, 1000.0F);
            auto light_view = Math::Mat4f::look_at(light_position, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F });

            Renderer::FrameData const frame_data {
                .projection = m_camera.projection(),
                .view = m_camera.view(),
                .camera_position = m_camera.position(),
                .directional_light = Renderer::DirectionalLight {
                    .direction = light_direction,
                    .color = { 1.0F, 1.0F, 1.0F, 1.0F },
                    .space_matrix = light_projection * light_view
                }
            };
            Renderer::DeferredRenderer::SubmitInfo const submit_info {
                .frame_data = frame_data,
                .output_render_target = m_swapchain_render_targets[image_index].get(),
                .command_buffer = cmd,
                .render_items = render_items
            };
            m_deferred_renderer->submit(submit_info);

            m_swapchain->end_frame(frame.value());
        }
    }

    ~Sandbox()
    {
        if (m_swapchain != nullptr) {
            m_swapchain->wait_idle();
        }
    }
private:
    Sandbox() = default;

    std::unique_ptr<Platform::Window> m_window;
    std::unique_ptr<RHI::Device> m_graphics_device;
    std::unique_ptr<RHI::Swapchain> m_swapchain;
    std::vector<std::unique_ptr<RHI::RenderTarget>> m_swapchain_render_targets;
    Asset::AssetManager m_asset_manager = Asset::AssetManager("Resources/");
    std::unique_ptr<Renderer::ResourceManager> m_resource_manager;
    std::unique_ptr<Renderer::DeferredRenderer> m_deferred_renderer;
    Renderer::Model const* m_sponza;
    Renderer::Camera m_camera {};
    bool m_was_window_resized = false;
};

auto main() -> i32
{
    auto sandbox = Sandbox::create();
    if (!sandbox.has_value()) {
        std::println(stderr, "{}", sandbox.error());
        return EXIT_FAILURE;
    }
    sandbox.value()->run();
    return EXIT_SUCCESS;
}

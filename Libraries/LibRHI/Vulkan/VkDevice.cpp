/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <format>
#include <vector>

#include <Common/Expected.h>
#include <LibDebug/Logger.h>

#define VMA_IMPLEMENTATION
#include <LibRHI/Vulkan/VkBuffer.h>
#include <LibRHI/Vulkan/VkCommon.h>
#include <LibRHI/Vulkan/VkDevice.h>
#include <LibRHI/Vulkan/VkPipeline.h>
#include <LibRHI/Vulkan/VkRenderPass.h>
#include <LibRHI/Vulkan/VkRenderTarget.h>
#include <LibRHI/Vulkan/VkResourceLayout.h>
#include <LibRHI/Vulkan/VkResourceSet.h>
#include <LibRHI/Vulkan/VkSampler.h>
#include <LibRHI/Vulkan/VkShader.h>
#include <LibRHI/Vulkan/VkSwapchain.h>
#include <LibRHI/Vulkan/VkTexture.h>
#ifdef OA_OS_WINDOWS
#    include <LibPlatform/Win32/WindowWin32.h>
#endif

namespace RHI {

namespace {

constexpr Debug::Logger Logger("Vulkan RHI");

auto to_log_level(VkDebugUtilsMessageSeverityFlagBitsEXT severity) -> Debug::LogLevel
{
    switch (severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        return Debug::LogLevel::Trace;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        return Debug::LogLevel::Debug;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        return Debug::LogLevel::Warn;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        return Debug::LogLevel::Error;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
        break;
    }
    return Debug::LogLevel::Debug;
}

auto to_string(VkDebugUtilsMessageTypeFlagsEXT message_type) -> std::string_view
{
    if ((message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0) {
        return "Validation";
    }
    if ((message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0) {
        return "Performance";
    }
    return "General";
}

}

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT message_type, VkDebugUtilsMessengerCallbackDataEXT const* callback_data, [[maybe_unused]] void* user_data)
{
    Logger.log(to_log_level(severity), "{}: {}", to_string(message_type), callback_data->pMessage);
    return VK_FALSE;
}

auto VkDevice::create(Configuration const& config) -> Common::Expected<std::unique_ptr<VkDevice>>
{
    std::unique_ptr<VkDevice> device(new VkDevice(config));

    return device->create_instance()
        .and_then([&]() {
            return device->create_surface();
        })
        .and_then([&]() {
            return device->create_logical_device();
        })
        .and_then([&]() {
            return device->create_allocator();
        })
        .and_then([&]() {
            return device->create_command_pools();
        })
        .and_then([&]() {
            return device->create_descriptor_pool();
        })
        .transform([&]() {
            return std::move(device);
        });
}

VkDevice::VkDevice(Configuration const& config)
    : m_config(config)
{
    assert(m_config.window != nullptr);
}

VkDevice::~VkDevice()
{
    for (auto* descriptor_pool : m_descriptor_pools) {
        vkDestroyDescriptorPool(m_logical_device, descriptor_pool, nullptr);
    }
    if (m_graphics_command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_logical_device, m_graphics_command_pool, nullptr);
    }
    if (m_transfer_command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_logical_device, m_transfer_command_pool, nullptr);
    }
    if (m_allocator != nullptr) {
        vmaDestroyAllocator(m_allocator);
    }
    if (m_surface != nullptr) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }
    if (m_debug_messenger != nullptr) {
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
        func(m_instance, m_debug_messenger, nullptr);
    }
    if (m_logical_device != nullptr) {
        vkDestroyDevice(m_logical_device, nullptr);
    }
    if (m_instance != nullptr) {
        vkDestroyInstance(m_instance, nullptr);
    }
}

auto VkDevice::descriptor_pool() const -> VkDescriptorPool
{
    return m_descriptor_pools.back();
}

auto VkDevice::grow_descriptor_pool() -> Common::Expected<VkDescriptorPool>
{
    m_descriptor_pool_capacity *= 2;
    return create_descriptor_pool()
        .transform([&]() {
            return m_descriptor_pools.back();
        });
}

auto VkDevice::allocator() const -> VmaAllocator
{
    return m_allocator;
}

auto VkDevice::handle() const -> ::VkDevice
{
    return m_logical_device;
}

auto VkDevice::surface() const -> VkSurfaceKHR
{
    return m_surface;
}

void VkDevice::submit_graphics(const RHI::VkCommandBuffer& command_buffer) const
{
    command_buffer.end();
    auto handle = command_buffer.handle();
    VkSubmitInfo const submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &handle,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };

    vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphics_queue);
}

auto VkDevice::graphics_command_buffer() const -> RHI::VkCommandBuffer const&
{
    return m_graphics_command_buffer;
}

auto VkDevice::graphics_queue() const -> VkQueue
{
    return m_graphics_queue;
}

auto VkDevice::present_queue() const -> VkQueue
{
    return m_present_queue;
}

auto VkDevice::transfer_queue() const -> VkQueue
{
    return m_transfer_queue;
}

auto VkDevice::graphics_pool() const -> VkCommandPool
{
    return m_graphics_command_pool;
}

auto VkDevice::transfer_pool() const -> VkCommandPool
{
    return m_transfer_command_pool;
}

auto VkDevice::selected_physical_device() const -> VkPhysicalDevice const*
{
    return m_physical_device;
}

auto VkDevice::physical_devices() const -> std::vector<std::string_view>
{
    std::vector<std::string_view> device_names;
    for (auto const& [name, _] : m_physical_devices) {
        device_names.push_back(name);
    }
    return device_names;
}

auto VkDevice::select_physical_device(std::string_view name) -> bool
{
    (void)name;
    return true;
}

auto VkDevice::create_instance() -> Common::Expected<void>
{
    VkApplicationInfo const app_info {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Omnia App",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "Omnia Engine",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    std::vector<char const*> required_extensions {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_PLATFORM_SURFACE_EXTENSION_NAME,
    };

    if (m_config.enable_debug_layer) {
        required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo instance_info {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<u32>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data(),
    };

    if (m_config.enable_debug_layer) {
        char const* instance_layers[] {
            "VK_LAYER_KHRONOS_validation"
        };

        u32 layer_count = 0;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        bool validation_layer_found = false;
        for (auto const& layerProperty : available_layers) {
            if (strcmp(layerProperty.layerName, instance_layers[0]) == 0) {
                validation_layer_found = true;
                break;
            }
        }
        if (!validation_layer_found) {
            return OA_ERROR("Vulkan validation layer requested but not available");
        }

        instance_info.enabledLayerCount = 1;
        instance_info.ppEnabledLayerNames = instance_layers;
    }

    if (auto result = vkCreateInstance(&instance_info, nullptr, &m_instance); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create Vulkan instance: {}", string_VkResult(result));
    }

    if (m_config.enable_debug_layer) {
        VkDebugUtilsMessengerCreateInfoEXT const debugMessengerCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = 0,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = vk_debug_callback,
            .pUserData = nullptr
        };

        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
        if (auto result = func(m_instance, &debugMessengerCreateInfo, nullptr, &m_debug_messenger); result != VK_SUCCESS) {
            return OA_ERROR("Failed to create Vulkan debug messenger: {}", string_VkResult(result));
        }
    }
    return {};
}

auto VkDevice::create_surface() -> Common::Expected<void>
{
#ifdef OA_OS_WINDOWS
    auto const* window = static_cast<Platform::WindowWin32 const*>(m_config.window);

    VkWin32SurfaceCreateInfoKHR const surface_info {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .hinstance = window->instance(),
        .hwnd = window->handle()
    };

    if (auto result = vkCreateWin32SurfaceKHR(m_instance, &surface_info, nullptr, &m_surface); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create Vulkan surface: {}", string_VkResult(result));
    }
#endif
    return {};
}

auto VkDevice::create_logical_device() -> Common::Expected<void>
{
    u32 device_count = 0;
    if (auto result = vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr); result != VK_SUCCESS) {
        return OA_ERROR("Failed to retrieve Vulkan physical devices: {}", string_VkResult(result));
    }
    if (device_count == 0) {
        return OA_ERROR("No Vulkan physical devices found");
    }
    std::vector<::VkPhysicalDevice> physical_devices(device_count);
    if (auto result = vkEnumeratePhysicalDevices(m_instance, &device_count, physical_devices.data()); result != VK_SUCCESS) {
        return OA_ERROR("Failed to retrieve Vulkan physical devices: {}", string_VkResult(result));
    }

    for (auto const& vk_device : physical_devices) {
        RHI::VkPhysicalDevice physical_device(vk_device, m_surface);
        if (physical_device.is_suitable()) {
            m_physical_devices[physical_device.name()] = std::move(physical_device);
        }
    }
    if (m_physical_devices.empty()) {
        return OA_ERROR("No suitable Vulkan physical devices found");
    }
    m_physical_device = &m_physical_devices.begin()->second;

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    f32 const queue_priority = 1.0F;
    auto const [graphics_index, present_index, transfer_index] = m_physical_device->queue_family_indices();

    queue_create_infos.push_back({ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = graphics_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority });

    queue_create_infos.push_back({ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = present_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority });

    queue_create_infos.push_back({ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = transfer_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority });

    VkPhysicalDeviceFeatures device_features {};
    device_features.fillModeNonSolid = VK_TRUE;

    std::vector<char const*> const required_extensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkDeviceCreateInfo const device_create_info {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = static_cast<u32>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<u32>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data(),
        .pEnabledFeatures = &device_features
    };

    if (auto result = vkCreateDevice(m_physical_device->handle(), &device_create_info, nullptr, &m_logical_device); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create Vulkan logical device: {}", string_VkResult(result));
    }
    vkGetDeviceQueue(m_logical_device, graphics_index, 0, &m_graphics_queue);
    vkGetDeviceQueue(m_logical_device, present_index, 0, &m_present_queue);
    vkGetDeviceQueue(m_logical_device, transfer_index, 0, &m_transfer_queue);
    return {};
}

auto VkDevice::create_allocator() -> Common::Expected<void>
{
    VmaAllocatorCreateInfo const allocator_info {
        .flags = 0,
        .physicalDevice = m_physical_device->handle(),
        .device = m_logical_device,
        .preferredLargeHeapBlockSize = 0,
        .pAllocationCallbacks = nullptr,
        .pDeviceMemoryCallbacks = nullptr,
        .pHeapSizeLimit = nullptr,
        .pVulkanFunctions = nullptr,
        .instance = m_instance,
        .vulkanApiVersion = VK_API_VERSION_1_3,
        .pTypeExternalMemoryHandleTypes = nullptr
    };

    if (auto result = vmaCreateAllocator(&allocator_info, &m_allocator); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create Vulkan memory allocator: {}", string_VkResult(result));
    }
    return {};
}

auto VkDevice::create_command_pools() -> Common::Expected<void>
{
    auto [graphics_index, present_index, transfer_index] = m_physical_device->queue_family_indices();

    VkCommandPoolCreateInfo const tranfer_pool_create_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = transfer_index
    };
    if (auto result = vkCreateCommandPool(m_logical_device, &tranfer_pool_create_info, nullptr, &m_transfer_command_pool); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create Vulkan graphics command pool: {}", string_VkResult(result));
    }

    VkCommandPoolCreateInfo const graphics_pool_create_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphics_index
    };
    if (auto result = vkCreateCommandPool(m_logical_device, &graphics_pool_create_info, nullptr, &m_graphics_command_pool); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create Vulkan graphics command pool: {}", string_VkResult(result));
    }

    m_graphics_command_buffer = TRY(VkCommandBuffer::create(m_graphics_command_pool, this));
    return {};
}

auto VkDevice::create_descriptor_pool() -> Common::Expected<void>
{
    std::vector<::VkDescriptorPoolSize> pool_sizes = {
        { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = m_descriptor_pool_capacity },
        { .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = m_descriptor_pool_capacity },
    };

    VkDescriptorPoolCreateInfo const descriptor_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = m_descriptor_pool_capacity,
        .poolSizeCount = static_cast<u32>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };

    ::VkDescriptorPool descriptor_pool {};
    if (auto result = vkCreateDescriptorPool(m_logical_device, &descriptor_pool_create_info, nullptr, &descriptor_pool); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create descriptor pool: {}", string_VkResult(result));
    }
    m_descriptor_pools.push_back(descriptor_pool);
    return {};
}

auto VkDevice::create_pipeline(Pipeline::Configuration const& config) const -> Common::Expected<std::unique_ptr<Pipeline>>
{
    return VkPipeline::create(config, this);
}

auto VkDevice::create_render_pass(RenderPass::Configuration const& config) const -> Common::Expected<std::unique_ptr<RenderPass>>
{
    return VkRenderPass::create(config, this);
}

auto VkDevice::create_render_target(RenderTarget::Configuration const& config) const -> Common::Expected<std::unique_ptr<RenderTarget>>
{
    return VkRenderTarget::create(config, this);
}

auto VkDevice::create_resource_layout(ResourceLayout::Configuration const& config) const -> Common::Expected<std::unique_ptr<ResourceLayout>>
{
    return VkResourceLayout::create(config, this);
}

auto VkDevice::create_resource_set(ResourceSet::Configuration const& config) -> Common::Expected<std::unique_ptr<ResourceSet>>
{
    return VkResourceSet::create(config, this);
}

auto VkDevice::create_sampler(Sampler::Configuration const& config) const -> Common::Expected<std::unique_ptr<Sampler>>
{
    return VkSampler::create(config, this);
}

auto VkDevice::create_buffer(Buffer::Configuration const& config) const -> Common::Expected<std::unique_ptr<Buffer>>
{
    return VkBuffer::create(config, this);
}

auto VkDevice::create_shader(Shader::Configuration const& config) const -> Common::Expected<std::unique_ptr<Shader>>
{
    return VkShader::create(config, this);
}

auto VkDevice::create_swapchain(Swapchain::Configuration const& config) const -> Common::Expected<std::unique_ptr<Swapchain>>
{
    return VkSwapchain::create(config, this);
}

auto VkDevice::create_texture(Texture::Configuration const& config) const -> Common::Expected<std::unique_ptr<Texture>>
{
    return VkTexture::create_owned(config, this);
}

}

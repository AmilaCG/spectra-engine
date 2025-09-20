//
// Created by Amila Abeygunasekara on Sun 20/07/2025.
//

#include "Context.h"

#include <format>
#include <stdexcept>
#include <iostream>

#include "Error.h"

namespace spectra::vk {
Context::~Context()
{
    if (instance != VK_NULL_HANDLE)
    {
        std::cerr << "spectra::vk::Context has not been de-initialized!\n";
        std::abort();
    }
}

void Context::init()
{
    vkb::InstanceBuilder builder;
    auto instanceRet = builder.set_app_name("Spectra Engine")
                              .require_api_version(1, 4)
                              .request_validation_layers()
                              .use_default_debug_messenger()
                              .build();
    if (!instanceRet)
    {
        throw std::runtime_error(std::format("Failed to create Vulkan instance: {}\n", instanceRet.error().message()));
    }
    vkbInstance_ = instanceRet.value();
    instance = vkbInstance_.instance;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Do not create an OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Disable window resizing for now
    pWindow = glfwCreateWindow(1280, 720, "Spectra Engine", nullptr, nullptr);

    VkResult glfwResult = glfwCreateWindowSurface(vkbInstance_, pWindow, nullptr, &surface);
    if (glfwResult != VK_SUCCESS)
    {
        throw std::runtime_error(std::format("Failed to create Vulkan surface: {}\n", std::to_string(glfwResult)));
    }

    vkb::PhysicalDeviceSelector selector(vkbInstance_);
    auto physicalDeviceRet = selector
                             .set_surface(surface)
                             .set_minimum_version(1, 4)
                             .select();
    if (!physicalDeviceRet)
    {
        throw std::runtime_error(std::format("Failed to select physical device: {}\n", physicalDeviceRet.error().message()));
    }
    const auto& vkbPhysicalDevice = physicalDeviceRet.value();
    physicalDevice = vkbPhysicalDevice.physical_device;

    VkPhysicalDeviceVulkan13Features vk13Features {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice);
    auto deviceRet = deviceBuilder
                     .add_pNext(&vk13Features)
                     .build();
    if (!deviceRet)
    {
        throw std::runtime_error(std::format("Failed to create physical device: {}\n", deviceRet.error().message()));
    }
    vkbDevice = deviceRet.value();
    device = vkbDevice.device;

    auto graphicsQueueRet = vkbDevice.get_queue(vkb::QueueType::graphics);
    if (!graphicsQueueRet)
    {
        throw std::runtime_error(std::format("Failed to get graphics queue: {}\n", graphicsQueueRet.error().message()));
    }
    graphicsQueue = graphicsQueueRet.value();

    auto presentQueueRet = vkbDevice.get_queue(vkb::QueueType::present);
    if (!presentQueueRet)
    {
        throw std::runtime_error(std::format("Failed to get present queue: {}\n", presentQueueRet.error().message()));
    }
    presentQueue = presentQueueRet.value();

    vkb::SwapchainBuilder swapchainBuilder(vkbDevice);
    auto swapchainRet = swapchainBuilder.build();
    if (!swapchainRet)
    {
        throw std::runtime_error(std::format("Failed to create swapchain: {}\n", swapchainRet.error().message()));
    }
    vkbSwapchain = swapchainRet.value();
    swapchain = vkbSwapchain.swapchain;
}

void Context::deinit()
{
    vkb::destroy_swapchain(vkbSwapchain);
    vkb::destroy_device(vkbDevice);
    vkb::destroy_surface(vkbInstance_, surface);
    vkb::destroy_instance(vkbInstance_);
    instance = VK_NULL_HANDLE;
    glfwDestroyWindow(pWindow);
    glfwTerminate();
}
} // spectra::vk
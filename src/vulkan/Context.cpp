//
// Created by Amila Abeygunasekara on Sun 20/07/2025.
//

#include "Context.h"

#include <format>
#include <stdexcept>
#include <VkBootstrap.h>

namespace vkpbr::vk {

Context::Context(GLFWwindow* window)
{
    vkb::InstanceBuilder builder;
    auto instance = builder.set_app_name("PBR Engine")
                           .request_validation_layers()
                           .use_default_debug_messenger()
                           .build();
    if (!instance)
    {
        throw std::runtime_error(std::format("Failed to create Vulkan instance {}\n", instance.error().message()));
    }
    vkb::Instance vkbInstance = instance.value();

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult glfwResult = glfwCreateWindowSurface(vkbInstance, window, nullptr, &surface);
    if (glfwResult != VK_SUCCESS)
    {
        throw std::runtime_error(std::format("Failed to create Vulkan surface {}\n", std::to_string(glfwResult)));
    }
}

} // vkpbr
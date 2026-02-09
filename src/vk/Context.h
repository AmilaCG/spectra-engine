//
// Created by Amila Abeygunasekara on Sun 20/07/2025.
//

#ifndef CONTEXT_H
#define CONTEXT_H

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

namespace spectra::vk {
class Context {
public:
    Context();
    ~Context();

    GLFWwindow* pWindow = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    // TODO: Add queue and index into a struct
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkSurfaceKHR surface = VK_NULL_HANDLE;

    vkb::Device vkbDevice{};

private:
    vkb::Instance vkbInstance_{};
};

} // spectra::vk

#endif //CONTEXT_H

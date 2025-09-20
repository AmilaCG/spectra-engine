//
// Created by Amila Abeygunasekara on Sun 20/07/2025.
//

#ifndef CONTEXT_H
#define CONTEXT_H

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>

namespace spectra::vk {

class Context {
public:
    ~Context();

    void init();
    void deinit();

    GLFWwindow* pWindow = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    // TODO: Add queue and index into a struct
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIdx = 0;
    VkQueue presentQueue = VK_NULL_HANDLE;
    uint32_t presentQueueFamilyIdx = 0;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    vkb::Swapchain vkbSwapchain{};

private:
    vkb::Instance vkbInstance_{};
    vkb::Device vkbDevice_{};
};

} // spectra::vk

#endif //CONTEXT_H

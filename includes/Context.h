//
// Created by Amila Abeygunasekara on Sun 20/07/2025.
//

#ifndef CONTEXT_H
#define CONTEXT_H

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>

namespace vkpbr::vk {

class Context {
public:
    Context();
    ~Context();

public:
    GLFWwindow* pWindow = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

private:
    vkb::Instance vkbInstance_{};
    vkb::Device vkbDevice_{};
    vkb::Swapchain vkbSwapchain_{};
};

} // vkpbr::vk

#endif //CONTEXT_H

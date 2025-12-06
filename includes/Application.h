//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include "Renderer.h"

namespace spectra {

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    void createSwapchain();
    void setupImGui();

    static void createTemporaryCommandPool(VkDevice device, uint32_t queueIndex, VkCommandPool& cmdPool);
    static void startOneTimeCommands(VkCommandBuffer& cb, VkDevice device, VkCommandPool cmdPool);
    static void endOneTimeCommands(VkCommandBuffer& cb, VkDevice device, VkCommandPool cmdPool, VkQueue queue);

    VkCommandPool temporaryCmdPool_ = VK_NULL_HANDLE;
    VkDescriptorPool imguiDescriptorPool_ = VK_NULL_HANDLE;

    vkb::Swapchain vkbSwapchain_{};
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;

    std::shared_ptr<vk::Context> pCtx_;
    std::unique_ptr<Renderer> pRenderer_;
};

} // spectra

#endif //APPLICATION_H

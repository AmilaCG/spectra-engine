//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#ifndef RENDERER_H
#define RENDERER_H

#include <memory>

#include "Context.h"
#include "ShaderModule.h"

namespace spectra {
class Renderer {
public:
    Renderer();

    void start();

private:
    void render();
    void shutdown();
    void createGraphicsPipeline();
    void createCommandPool(VkCommandPool& commandPool);
    void createSwapchain();
    void allocateCommandBuffers(VkDevice device);
    void createSyncObjects(VkDevice device);

    std::unique_ptr<vk::Context>        pCtx_;

    std::unique_ptr<vk::ShaderModule>   pShaderTriangleVert_;
    std::unique_ptr<vk::ShaderModule>   pShaderTriangleFrag_;

    VkPipelineLayout graphicsPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;

    VkCommandPool commandPool_ = VK_NULL_HANDLE;

    vkb::Swapchain vkbSwapchain_{};
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;

    std::vector<VkCommandBuffer> commandBuffers_;

    std::vector<VkSemaphore> availableSemaphores_;
    std::vector<VkSemaphore> finishedSemaphores_;
    std::vector<VkFence> inFlightFences_; // Per frame
    std::vector<VkFence> swapchainImgFences_; // Per swapchain image
};
} // spectra

#endif //RENDERER_H

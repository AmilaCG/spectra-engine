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
    void recordCommandBuffers();
    void setupImGui();

    static void transitionImageLayout(
        VkCommandBuffer cb,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags2 srcStage,
        VkAccessFlags2 srcAccess,
        VkPipelineStageFlags2 dstStage,
        VkAccessFlags2 dstAccess,
        uint32_t srcQueueFamily = VK_QUEUE_FAMILY_IGNORED,
        uint32_t dstQueueFamily = VK_QUEUE_FAMILY_IGNORED);

    std::unique_ptr<vk::Context>        pCtx_;

    std::unique_ptr<vk::ShaderModule>   pShaderTriangleVert_;
    std::unique_ptr<vk::ShaderModule>   pShaderTriangleFrag_;

    VkPipelineLayout graphicsPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;

    VkViewport viewport_{};
    VkRect2D scissor_{};

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

    VkDescriptorPool imguiDescriptorPool_ = VK_NULL_HANDLE;

    uint32_t currentFrame_ = 0;
};
} // spectra

#endif //RENDERER_H

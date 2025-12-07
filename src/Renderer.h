//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#ifndef RENDERER_H
#define RENDERER_H

#include <memory>
#include <tiny_gltf.h>

#include "vulkan/Context.h"
#include "vulkan/ShaderModule.h"

namespace spectra {
class Renderer {
public:
    Renderer(std::shared_ptr<vk::Context> pCtx, vkb::Swapchain swapchain, std::vector<VkImageView> swapchainImgViews);

    void loadScene(const std::string& scenePath);
    void render();
    void shutdown();

private:
    void createGraphicsPipeline();
    void createCommandPool(VkCommandPool& commandPool);
    void allocateCommandBuffers(VkDevice device);
    void createSyncObjects(VkDevice device);
    void recordCommandBuffer(uint32_t imageIndex);

    std::shared_ptr<vk::Context>        pCtx_;

    std::unique_ptr<vk::ShaderModule>   pShaderTriangleVert_;
    std::unique_ptr<vk::ShaderModule>   pShaderTriangleFrag_;

    VkPipelineLayout graphicsPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;

    VkViewport viewport_{};
    VkRect2D scissor_{};

    vkb::Swapchain vkbSwapchain_{};
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;

    std::vector<VkSemaphore> availableSemaphores_;
    std::vector<VkSemaphore> finishedSemaphores_;
    std::vector<VkFence> inFlightFences_; // Per frame
    std::vector<VkFence> swapchainImgFences_; // Per swapchain image

    VkDescriptorPool imguiDescriptorPool_ = VK_NULL_HANDLE;

    uint32_t currentFrame_ = 0;

    struct FrameData
    {
        VkCommandPool cmdPool = VK_NULL_HANDLE;
        VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
    };
    std::vector<FrameData> frames_{};

    tinygltf::Model model_;
};
} // spectra

#endif //RENDERER_H

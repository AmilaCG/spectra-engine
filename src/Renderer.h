//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#ifndef RENDERER_H
#define RENDERER_H

#include <memory>
#include <tiny_gltf.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

#include "vk/Context.h"

namespace spectra {
class Renderer {
public:
    Renderer(std::shared_ptr<vk::Context> pCtx, vkb::Swapchain swapchain, std::vector<VkImageView> swapchainImgViews);
    ~Renderer();

    void loadScene(const std::string& scenePath);
    void render();

private:
    void initVma();
    void createBuffers(const tinygltf::Model& model);
    void createGraphicsPipeline();
    void createCommandPool(VkCommandPool& commandPool);
    void allocateCommandBuffers(VkDevice device);
    void createSyncObjects(VkDevice device);
    void recordCommandBuffer(VkCommandBuffer cb, uint32_t imgIndex) const;
    Slang::ComPtr<slang::ISession> setupSlangSession(const Slang::ComPtr<slang::IGlobalSession>& globalSession) const;

    std::shared_ptr<vk::Context>        pCtx_;
    VkDevice                            device_ = VK_NULL_HANDLE;

    VmaAllocator                        allocator_ = VK_NULL_HANDLE;

    Slang::ComPtr<slang::IGlobalSession> slangGlobalSession_{};

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

    struct Vertex
    {
        glm::vec2 position;
        glm::vec3 color;
    };

    VkBuffer vertBuffer_ = VK_NULL_HANDLE;
    VmaAllocation vertAlloc_{};
    VkBuffer indexBuffer_ = VK_NULL_HANDLE;
    VmaAllocation indexAlloc_{};
    VkBuffer stagingBuffer_ = VK_NULL_HANDLE;
    VmaAllocation stagingAlloc_{};

    VkCommandPool temporaryCmdPool_ = VK_NULL_HANDLE;
};
} // spectra

#endif //RENDERER_H

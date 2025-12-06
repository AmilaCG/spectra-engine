//
// Created by Amila Abeygunasekara on Sat 08/09/2025.
//

#ifndef VK_PBR_ENGINE_UTILITIES_H
#define VK_PBR_ENGINE_UTILITIES_H

#include <vulkan/vulkan.h>

namespace spectra::utils {
namespace vk {

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
    uint32_t dstQueueFamily = VK_QUEUE_FAMILY_IGNORED)
{
    VkImageMemoryBarrier2 imageBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = srcStage,
        .srcAccessMask = srcAccess,
        .dstStageMask = dstStage,
        .dstAccessMask = dstAccess,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = srcQueueFamily,
        .dstQueueFamilyIndex = dstQueueFamily,
        .image = image,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    };

    VkDependencyInfo dependencyInfo {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier
    };

    vkCmdPipelineBarrier2(cb, &dependencyInfo);
}

static void createTemporaryCommandPool(VkDevice device, uint32_t queueIndex, VkCommandPool& cmdPool)
{
    const VkCommandPoolCreateInfo commandPoolCreateInfo{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,  // Commands will be short-lived
        .queueFamilyIndex = queueIndex
    };
    CHECK_VK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &cmdPool));
}

static void beginOneTimeCommands(VkCommandBuffer& cb, VkDevice device, VkCommandPool cmdPool)
{
    const VkCommandBufferAllocateInfo allocInfo{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = cmdPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    CHECK_VK(vkAllocateCommandBuffers(device, &allocInfo, &cb));

    const VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    CHECK_VK(vkBeginCommandBuffer(cb, &beginInfo));
}

static void endOneTimeCommands(VkCommandBuffer& cb, VkDevice device, VkCommandPool cmdPool, VkQueue queue)
{
    CHECK_VK(vkEndCommandBuffer(cb));

    constexpr VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    std::array<VkFence, 1>  fence{};
    CHECK_VK(vkCreateFence(device, &fenceInfo, nullptr, fence.data()));

    const VkCommandBufferSubmitInfo cmdBufferInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cb
    };

    const std::array<VkSubmitInfo2, 1> submitInfo{
        {{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &cmdBufferInfo}},
        };
    CHECK_VK(vkQueueSubmit2(queue, submitInfo.size(), submitInfo.data(), fence[0]));
    CHECK_VK(vkWaitForFences(device, fence.size(), fence.data(), VK_TRUE, UINT64_MAX));

    // Cleanup
    vkDestroyFence(device, fence[0], nullptr);
    vkFreeCommandBuffers(device, cmdPool, 1, &cb);
}

} // vk
} // spectra::utils

#endif //VK_PBR_ENGINE_UTILITIES_H
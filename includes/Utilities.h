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

} // vk
} // spectra::utils

#endif //VK_PBR_ENGINE_UTILITIES_H
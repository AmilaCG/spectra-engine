//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Renderer.h"

#include <utility>

#include "Error.h"

#include "triangle.vert.h"
#include "triangle.frag.h"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

namespace spectra {
Renderer::Renderer()
{
    pCtx_ = std::make_unique<vk::Context>();
    pCtx_->init();

    createCommandPool(commandPool_);
    createSwapchain();
    createGraphicsPipeline();
    allocateCommandBuffers(pCtx_->device);
    createSyncObjects(pCtx_->device);
    recordCommandBuffers();
}

void Renderer::start()
{
    // Main loop
    while (!glfwWindowShouldClose(pCtx_->pWindow))
    {
        glfwPollEvents();

        render();
    }
    vkDeviceWaitIdle(pCtx_->device);

    shutdown();
}

void Renderer::render()
{
    vkWaitForFences(pCtx_->device, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        pCtx_->device, swapchain_, UINT64_MAX, availableSemaphores_[currentFrame_], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // TODO: Recreate swapchain
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        std::cerr << "Failed to acquire swapchain image!\n";
        CHECK_VK(result)
    }

    if (swapchainImgFences_[imageIndex] != VK_NULL_HANDLE)
    {
        CHECK_VK(vkWaitForFences(pCtx_->device, 1, &swapchainImgFences_[imageIndex], VK_TRUE, UINT64_MAX))
    }
    swapchainImgFences_[imageIndex] = swapchainImgFences_[currentFrame_];

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    };

    VkSemaphore waitSemaphores[] = { availableSemaphores_[currentFrame_] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];

    VkSemaphore signalSemaphores[] = { finishedSemaphores_[imageIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    CHECK_VK(vkResetFences(pCtx_->device, 1, &inFlightFences_[currentFrame_]))

    CHECK_VK(vkQueueSubmit(pCtx_->graphicsQueue, 1, &submitInfo, inFlightFences_[currentFrame_]))

    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = &swapchain_,
        .pImageIndices = &imageIndex,
    };

    CHECK_VK(vkQueuePresentKHR(pCtx_->presentQueue, &presentInfo))

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::shutdown()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(pCtx_->device, availableSemaphores_[i], VK_NULL_HANDLE);
        vkDestroyFence(pCtx_->device, inFlightFences_[i], VK_NULL_HANDLE);
    }

    for (uint32_t i = 0; i < vkbSwapchain_.image_count; i++)
    {
        vkDestroySemaphore(pCtx_->device, finishedSemaphores_[i], VK_NULL_HANDLE);
        vkDestroyFence(pCtx_->device, swapchainImgFences_[i], VK_NULL_HANDLE);
    }

    // TODO: Have a separate class for pipelines and handle lifecycles from there
    vkDestroyPipeline(pCtx_->device, graphicsPipeline_, nullptr);
    vkDestroyPipelineLayout(pCtx_->device, graphicsPipelineLayout_, nullptr);

    vkDestroyCommandPool(pCtx_->device, commandPool_, nullptr);

    vkbSwapchain_.destroy_image_views(swapchainImageViews_);
    vkb::destroy_swapchain(vkbSwapchain_);

    pCtx_->deinit();
}

void Renderer::createGraphicsPipeline()
{
    pShaderTriangleVert_ = std::make_unique<vk::ShaderModule>(pCtx_->device, triangle_vert);
    pShaderTriangleFrag_ = std::make_unique<vk::ShaderModule>(pCtx_->device, triangle_frag);

    if (pShaderTriangleVert_->value() == VK_NULL_HANDLE || pShaderTriangleFrag_->value() == VK_NULL_HANDLE)
    {
        std::cerr << "Failed to create shader modules\n";
    }

    VkPipelineShaderStageCreateInfo vertStageInfo = {};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = pShaderTriangleVert_->value();
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo = {};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = pShaderTriangleFrag_->value();
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    viewport_.x = 0.0f;
    viewport_.y = 0.0f;
    viewport_.width = (float)vkbSwapchain_.extent.width;
    viewport_.height = (float)vkbSwapchain_.extent.height;
    viewport_.minDepth = 0.0f;
    viewport_.maxDepth = 1.0f;

    scissor_.offset = { 0, 0 };
    scissor_.extent = vkbSwapchain_.extent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport_;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor_;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachment;
    colorBlendState.blendConstants[0] = 0.0f;
    colorBlendState.blendConstants[1] = 0.0f;
    colorBlendState.blendConstants[2] = 0.0f;
    colorBlendState.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 0;
    layoutCreateInfo.pushConstantRangeCount = 0;

    // TODO: Name vulkan objects to identify them in validation messages
    CHECK_VK(vkCreatePipelineLayout(pCtx_->device, &layoutCreateInfo, nullptr, &graphicsPipelineLayout_))

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineRenderingCreateInfo pipelineRenderingInfo = {};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.pNext = VK_NULL_HANDLE;
    pipelineRenderingInfo.pColorAttachmentFormats = &vkbSwapchain_.image_format;
    pipelineRenderingInfo.depthAttachmentFormat   = VK_FORMAT_UNDEFINED;
    pipelineRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = graphicsPipelineLayout_;
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.pNext = &pipelineRenderingInfo;

    CHECK_VK(vkCreateGraphicsPipelines(pCtx_->device, VK_NULL_HANDLE, 1, &pipelineInfo, VK_NULL_HANDLE, &graphicsPipeline_))

    pShaderTriangleVert_->destroy();
    pShaderTriangleFrag_->destroy();
}

void Renderer::createCommandPool(VkCommandPool& commandPool)
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = pCtx_->vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    CHECK_VK(vkCreateCommandPool(pCtx_->device, &createInfo, VK_NULL_HANDLE, &commandPool))
}

void Renderer::createSwapchain()
{
    vkb::SwapchainBuilder swapchainBuilder(pCtx_->vkbDevice);
    auto builderRet = swapchainBuilder.set_old_swapchain(swapchain_).build();
    if (!builderRet)
    {
        std::cerr << builderRet.error().message() << " " << builderRet.vk_result() << "\n";
    }

    vkb::destroy_swapchain(vkbSwapchain_);
    vkbSwapchain_ = builderRet.value();
    swapchain_ = vkbSwapchain_.swapchain;

    swapchainImages_ = vkbSwapchain_.get_images().value();
    swapchainImageViews_ = vkbSwapchain_.get_image_views().value();
}

void Renderer::allocateCommandBuffers(VkDevice device)
{
    commandBuffers_.resize(swapchainImageViews_.size());

    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = commandPool_;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAllocInfo.commandBufferCount = commandBuffers_.size();

    CHECK_VK(vkAllocateCommandBuffers(device, &cbAllocInfo, commandBuffers_.data()))
}

void Renderer::createSyncObjects(VkDevice device)
{
    availableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    finishedSemaphores_.resize(vkbSwapchain_.image_count);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);
    swapchainImgFences_.resize(vkbSwapchain_.image_count);

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < vkbSwapchain_.image_count; i++)
    {
        CHECK_VK(vkCreateSemaphore(device, &semaphoreCreateInfo, VK_NULL_HANDLE, &finishedSemaphores_[i]))
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        CHECK_VK(vkCreateSemaphore(device, &semaphoreCreateInfo, VK_NULL_HANDLE, &availableSemaphores_[i]))
        CHECK_VK(vkCreateFence(device, &fenceCreateInfo, VK_NULL_HANDLE, &inFlightFences_[i]))
    }
}

void Renderer::recordCommandBuffers()
{
    for (uint32_t i = 0; i < swapchainImageViews_.size(); i++)
    {
        VkCommandBuffer cb = commandBuffers_[i];

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        CHECK_VK(vkBeginCommandBuffer(cb, &beginInfo))

        VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };

        VkRenderingAttachmentInfo renderingAttachmentInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchainImageViews_[i],
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = clearColor,
        };

        VkRenderingInfo renderingInfo = {};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = scissor_;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &renderingAttachmentInfo;

        vkCmdSetViewport(cb, 0, 1, &viewport_);
        vkCmdSetScissor(cb, 0, 1, &scissor_);

        transitionImageLayout(cb,
            swapchainImages_[i],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_NONE,
            VK_ACCESS_NONE,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
        );

        vkCmdBeginRendering(cb, &renderingInfo);

        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);
        vkCmdDraw(cb, 3, 1, 0, 0);

        vkCmdEndRendering(cb);

        transitionImageLayout(cb,
            swapchainImages_[i],
            VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_NONE,
            VK_ACCESS_NONE
        );

        CHECK_VK(vkEndCommandBuffer(cb))
    }
}

void Renderer::transitionImageLayout(
    VkCommandBuffer cb,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkPipelineStageFlags2 srcStage,
    VkAccessFlags2 srcAccess,
    VkPipelineStageFlags2 dstStage,
    VkAccessFlags2 dstAccess,
    uint32_t srcQueueFamily,
    uint32_t dstQueueFamily)
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
} // spectra
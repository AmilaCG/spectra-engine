//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Renderer.h"

#include <array>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "Error.h"

#include "triangle.vert.h"
#include "triangle.frag.h"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

namespace spectra {
Renderer::Renderer()
{
    pCtx_ = std::make_unique<vk::Context>();
    pCtx_->init();

    frames_.resize(MAX_FRAMES_IN_FLIGHT);

    createTemporaryCommandPool(pCtx_->device, pCtx_->vkbDevice.get_queue_index(vkb::QueueType::graphics).value(), temporaryCmdPool_);
    createSwapchain();
    createGraphicsPipeline();
    allocateCommandBuffers(pCtx_->device);
    createSyncObjects(pCtx_->device);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    setupImGui();
}

void Renderer::start()
{
    // Main loop
    while (!glfwWindowShouldClose(pCtx_->pWindow))
    {
        glfwPollEvents();

        // Build ImGui frame and UI
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplVulkan_NewFrame();
        ImGui::NewFrame();

        render();

        ImGui::EndFrame();
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
    swapchainImgFences_[imageIndex] = inFlightFences_[currentFrame_];

    CHECK_VK(vkResetFences(pCtx_->device, 1, &inFlightFences_[currentFrame_]))

    ImGui::Begin("Stats");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Render();

    // Record commands for this image (includes triangle + ImGui)
    recordCommandBuffer(imageIndex);

    VkSemaphoreSubmitInfo waitSemaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = availableSemaphores_[currentFrame_],
        .stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    VkSemaphoreSubmitInfo signalSemaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = finishedSemaphores_[imageIndex],
        .stageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    };

    VkCommandBufferSubmitInfo cmdSubmitInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = frames_[imageIndex].cmdBuffer,
    };

    VkSubmitInfo2 submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdSubmitInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signalSemaphoreInfo,
    };

    CHECK_VK(vkQueueSubmit2(pCtx_->graphicsQueue, 1, &submitInfo, inFlightFences_[currentFrame_]))

    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &finishedSemaphores_[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &swapchain_,
        .pImageIndices = &imageIndex,
    };

    CHECK_VK(vkQueuePresentKHR(pCtx_->presentQueue, &presentInfo))

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::shutdown()
{
    // Shutdown ImGui before destroying the descriptor pool it uses
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(pCtx_->device, imguiDescriptorPool_, nullptr);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(pCtx_->device, availableSemaphores_[i], VK_NULL_HANDLE);
        vkDestroyFence(pCtx_->device, inFlightFences_[i], VK_NULL_HANDLE);
    }

    for (uint32_t i = 0; i < vkbSwapchain_.image_count; i++)
    {
        vkDestroySemaphore(pCtx_->device, finishedSemaphores_[i], VK_NULL_HANDLE);
        // swapchainImgFences_ aliases inFlightFences_ and is not owned; no need to destroy it here
    }

    // TODO: Have a separate class for pipelines and handle lifecycles from there
    vkDestroyPipeline(pCtx_->device, graphicsPipeline_, nullptr);
    vkDestroyPipelineLayout(pCtx_->device, graphicsPipelineLayout_, nullptr);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyCommandPool(pCtx_->device, frames_[i].cmdPool, nullptr);
    }
    vkDestroyCommandPool(pCtx_->device, temporaryCmdPool_, nullptr);

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
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = pCtx_->vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    CHECK_VK(vkCreateCommandPool(pCtx_->device, &createInfo, VK_NULL_HANDLE, &commandPool))
}

void Renderer::createTemporaryCommandPool(VkDevice device, uint32_t queueIndex, VkCommandPool& cmdPool)
{
    const VkCommandPoolCreateInfo commandPoolCreateInfo{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,  // Commands will be short-lived
        .queueFamilyIndex = queueIndex
    };
    CHECK_VK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &cmdPool));
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

    VkCommandBuffer cmd{};
    startOneTimeCommands(cmd, pCtx_->device, temporaryCmdPool_);

    // Set initial swapchain image layouts to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    for (int i = 0; i < vkbSwapchain_.image_count; i++)
    {
        transitionImageLayout(cmd,
            swapchainImages_[i],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_NONE,
            VK_ACCESS_NONE
        );
    }

    endOneTimeCommands(cmd, pCtx_->device, temporaryCmdPool_, pCtx_->graphicsQueue);
}

void Renderer::allocateCommandBuffers(VkDevice device)
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createCommandPool(frames_[i].cmdPool);

        VkCommandBufferAllocateInfo cbAllocInfo = {};
        cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbAllocInfo.commandPool = frames_[i].cmdPool;
        cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbAllocInfo.commandBufferCount = 1;

        CHECK_VK(vkAllocateCommandBuffers(device, &cbAllocInfo, &frames_[i].cmdBuffer))
    }
}

void Renderer::createSyncObjects(VkDevice device)
{
    availableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    finishedSemaphores_.resize(vkbSwapchain_.image_count);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);
    swapchainImgFences_.assign(vkbSwapchain_.image_count, VK_NULL_HANDLE);

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

void Renderer::recordCommandBuffer(uint32_t i)
{
    VkCommandBuffer cb = frames_[i].cmdBuffer;

    CHECK_VK(vkResetCommandBuffer(cb, 0))

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    CHECK_VK(vkBeginCommandBuffer(cb, &beginInfo))

    VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };

    VkRenderingAttachmentInfo renderingAttachmentInfo {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchainImageViews_[i],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_NONE,
        VK_ACCESS_NONE,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
    );

    vkCmdBeginRendering(cb, &renderingInfo);

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);
    vkCmdDraw(cb, 3, 1, 0, 0);

    // Render ImGui draw data within the same rendering scope
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb, VK_NULL_HANDLE);

    vkCmdEndRendering(cb);

    transitionImageLayout(cb,
        swapchainImages_[i],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_NONE,
        VK_ACCESS_NONE
    );

    CHECK_VK(vkEndCommandBuffer(cb))
}

void Renderer::startOneTimeCommands(VkCommandBuffer& cb, VkDevice device, VkCommandPool cmdPool)
{
    const VkCommandBufferAllocateInfo allocInfo{.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                .commandPool        = cmdPool,
                                                .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                .commandBufferCount = 1};
    CHECK_VK(vkAllocateCommandBuffers(device, &allocInfo, &cb));
    const VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                             .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    CHECK_VK(vkBeginCommandBuffer(cb, &beginInfo));

}

void Renderer::endOneTimeCommands(VkCommandBuffer& cb, VkDevice device, VkCommandPool cmdPool, VkQueue queue)
{
    CHECK_VK(vkEndCommandBuffer(cb));

    const VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    std::array<VkFence, 1>  fence{};
    CHECK_VK(vkCreateFence(device, &fenceInfo, nullptr, fence.data()));

    const VkCommandBufferSubmitInfo cmdBufferInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cb
    };

    const std::array<VkSubmitInfo2, 1> submitInfo{
              {{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .commandBufferInfoCount = 1, .pCommandBufferInfos = &cmdBufferInfo}},
    };
    CHECK_VK(vkQueueSubmit2(queue, submitInfo.size(), submitInfo.data(), fence[0]));
    CHECK_VK(vkWaitForFences(device, fence.size(), fence.data(), VK_TRUE, UINT64_MAX));

    // Cleanup
    vkDestroyFence(device, fence[0], nullptr);
    vkFreeCommandBuffers(device, cmdPool, 1, &cb);
}

void Renderer::setupImGui()
{
    constexpr uint32_t texturePoolSize = 128U;
    static VkFormat imageFormats = VK_FORMAT_B8G8R8A8_UNORM;  // Must be static for ImGui_ImplVulkan_InitInfo

    const std::array<VkDescriptorPoolSize, 1> poolSizes {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texturePoolSize}
    };

    const VkDescriptorPoolCreateInfo poolInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = texturePoolSize,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };

    CHECK_VK(vkCreateDescriptorPool(pCtx_->device, &poolInfo, nullptr, &imguiDescriptorPool_))

    constexpr ImGuiConfigFlags configFlags { ImGuiConfigFlags_NavEnableKeyboard };
    ImGuiIO& io    = ImGui::GetIO();
    io.ConfigFlags = configFlags;

    ImGui_ImplGlfw_InitForVulkan(pCtx_->pWindow, true);
    imageFormats = vkbSwapchain_.image_format;

    ImGui_ImplVulkan_InitInfo initInfo = {
        .ApiVersion                  = VK_API_VERSION_1_4,
        .Instance                    = pCtx_->instance,
        .PhysicalDevice              = pCtx_->physicalDevice,
        .Device                      = pCtx_->device,
        .QueueFamily                 = pCtx_->vkbDevice.get_queue_index(vkb::QueueType::graphics).value(),
        .Queue                       = pCtx_->vkbDevice.get_queue(vkb::QueueType::graphics).value(),
        .DescriptorPool              = imguiDescriptorPool_,
        .MinImageCount               = MAX_FRAMES_IN_FLIGHT,
        .ImageCount                  = std::max(vkbSwapchain_.image_count, MAX_FRAMES_IN_FLIGHT),
        .UseDynamicRendering         = true,
        .PipelineRenderingCreateInfo =
        {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount    = 1,
            .pColorAttachmentFormats = &imageFormats,
        },
    };

    ImGui_ImplVulkan_Init(&initInfo);
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
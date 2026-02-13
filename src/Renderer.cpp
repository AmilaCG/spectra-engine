//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Renderer.h"

#include <utility>
#include <backends/imgui_impl_vulkan.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "vk/Error.h"
#include "Utilities.h"

namespace spectra {
Renderer::Renderer(std::shared_ptr<vk::Context> pCtx, vkb::Swapchain swapchain, std::vector<VkImageView> swapchainImgViews)
    : pCtx_(pCtx), device_(pCtx->device), vkbSwapchain_(swapchain), swapchainImageViews_(std::move(swapchainImgViews))
{
    frames_.resize(MAX_FRAMES_IN_FLIGHT);

    swapchainImages_ = swapchain.get_images().value();

    // Connect with the Slang API
    slang::createGlobalSession(slangGlobalSession_.writeRef());

    utils::vk::createTemporaryCommandPool(
        device_, pCtx_->vkbDevice.get_queue_index(vkb::QueueType::graphics).value(), temporaryCmdPool_);

    initVma();
    createBuffers();
    createGraphicsPipeline();
    allocateCommandBuffers(device_);
    createSyncObjects(device_);
}

Renderer::~Renderer()
{
    vmaDestroyBuffer(allocator_, stagingBuffer_, stagingAlloc_);
    vmaDestroyBuffer(allocator_, vertBuffer_, vertAlloc_);

    vmaDestroyAllocator(allocator_);

    vkDestroyCommandPool(device_, temporaryCmdPool_, nullptr);

    for (const auto& semaphore : availableSemaphores_)
    {
        vkDestroySemaphore(device_, semaphore, VK_NULL_HANDLE);
    }
    for (const auto& fence : inFlightFences_)
    {
        vkDestroyFence(device_, fence, VK_NULL_HANDLE);
    }

    for (const auto& semaphore : finishedSemaphores_)
    {
        vkDestroySemaphore(device_, semaphore, VK_NULL_HANDLE);
        // swapchainImgFences_ aliases inFlightFences_ and is not owned; no need to destroy it here
    }

    // TODO: Have a separate class for pipelines and handle lifecycles from there
    vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
    vkDestroyPipelineLayout(device_, graphicsPipelineLayout_, nullptr);

    for (const auto& frame : frames_)
    {
        vkDestroyCommandPool(device_, frame.cmdPool, nullptr);
    }
}

void Renderer::loadScene(const std::string& scenePath)
{
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    // bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, modelPath);
    const bool ret = loader.LoadBinaryFromFile(&model_, &err, &warn, scenePath);

    if (!warn.empty())
    {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty())
    {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret)
    {
        printf("Failed to parse glTF: %s\n", scenePath.c_str());
    }
}

void Renderer::render()
{
    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        device_, vkbSwapchain_.swapchain, UINT64_MAX, availableSemaphores_[currentFrame_], VK_NULL_HANDLE, &imageIndex);

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
        CHECK_VK(vkWaitForFences(device_, 1, &swapchainImgFences_[imageIndex], VK_TRUE, UINT64_MAX))
    }
    swapchainImgFences_[imageIndex] = inFlightFences_[currentFrame_];

    CHECK_VK(vkResetFences(device_, 1, &inFlightFences_[currentFrame_]))

    ImGui::Begin("Stats");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Render();

    // Record commands for this image (includes triangle + ImGui)
    recordCommandBuffer(frames_[imageIndex].cmdBuffer, imageIndex);

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
        .pSwapchains = &vkbSwapchain_.swapchain,
        .pImageIndices = &imageIndex,
    };

    CHECK_VK(vkQueuePresentKHR(pCtx_->presentQueue, &presentInfo))

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::initVma()
{
    VmaVulkanFunctions vkFunctions
    {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkCreateImage = vkCreateImage
    };

    VmaAllocatorCreateInfo allocatorCreateInfo
    {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = pCtx_->physicalDevice,
        .device = device_,
        .pVulkanFunctions = &vkFunctions,
        .instance = pCtx_->instance,
    };

    CHECK_VK(vmaCreateAllocator(&allocatorCreateInfo, &allocator_));
}

void Renderer::createBuffers()
{
    const std::vector<Vertex> vertices
    {
        { glm::vec2( 0.0f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f) },
        { glm::vec2( 0.5f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f) },
        { glm::vec2(-0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f) },
    };

    const VkDeviceSize vertBufSize = vertices.size() * sizeof(Vertex);
    VkBufferCreateInfo vertBufferCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = vertBufSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    };
    VmaAllocationCreateInfo vertAllocCreateInfo
    {
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    };
    CHECK_VK(vmaCreateBuffer(allocator_, &vertBufferCreateInfo, &vertAllocCreateInfo, &vertBuffer_, &vertAlloc_, nullptr));

    VkBufferCreateInfo stagingBufferCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = vertBufSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    };
    VmaAllocationCreateInfo stagingAllocCreateInfo
    {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    CHECK_VK(vmaCreateBuffer(allocator_, &stagingBufferCreateInfo, &stagingAllocCreateInfo, &stagingBuffer_, &stagingAlloc_, nullptr));

    void* mapped = nullptr;
    CHECK_VK(vmaMapMemory(allocator_, stagingAlloc_, &mapped));
    memcpy(mapped, vertices.data(), vertBufSize);
    vmaUnmapMemory(allocator_, stagingAlloc_);

    CHECK_VK(vmaFlushAllocation(allocator_, stagingAlloc_, 0, vertBufSize));

    // Copy vertices to device on initialization
    VkCommandBuffer cmd{};
    utils::vk::beginOneTimeCommands(cmd, device_, temporaryCmdPool_);

    const VkBufferCopy copyRegion
    {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = vertBufSize
    };
    vkCmdCopyBuffer(cmd, stagingBuffer_, vertBuffer_, 1, &copyRegion);

    utils::vk::endOneTimeCommands(cmd, device_, temporaryCmdPool_, pCtx_->graphicsQueue);
}

void Renderer::createGraphicsPipeline()
{
    Slang::ComPtr<slang::ISession> slangSession = setupSlangSession(slangGlobalSession_);
    Slang::ComPtr<slang::IModule> slangModule = nullptr;
    slangModule = slangSession->loadModuleFromSource("triangle", "shaders/triangle.slang", nullptr, nullptr);
    Slang::ComPtr<ISlangBlob> spirv;
    slangModule->getTargetCode(0, spirv.writeRef());

    VkShaderModuleCreateInfo shaderModuleInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv->getBufferSize(),
        .pCode = static_cast<const uint32_t*>(spirv->getBufferPointer()),
    };

    VkShaderModule shaderModule;
    CHECK_VK(vkCreateShaderModule(device_, &shaderModuleInfo, nullptr, &shaderModule));

    VkPipelineShaderStageCreateInfo vertStageInfo = {};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = shaderModule,
    vertStageInfo.pName = "vertexMain";

    VkPipelineShaderStageCreateInfo fragStageInfo = {};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = shaderModule,
    fragStageInfo.pName = "fragmentMain";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

    VkVertexInputBindingDescription vertexBinding{};
    vertexBinding.binding = 0;
    vertexBinding.stride = sizeof(Vertex);
    vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> vertexAttributes = {
        {
            .location = 0,
            .binding = vertexBinding.binding,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = 0
        },
        {
            .location = 1,
            .binding = vertexBinding.binding,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = sizeof(Vertex::position)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBinding;
    vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributes.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

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
    CHECK_VK(vkCreatePipelineLayout(device_, &layoutCreateInfo, nullptr, &graphicsPipelineLayout_));

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineRenderingCreateInfo pipelineRenderingInfo = {};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.pNext = VK_NULL_HANDLE;
    pipelineRenderingInfo.colorAttachmentCount = 1;
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

    CHECK_VK(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, VK_NULL_HANDLE, &graphicsPipeline_));

    vkDestroyShaderModule(device_, shaderModule, nullptr);
}

void Renderer::createCommandPool(VkCommandPool& commandPool)
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = pCtx_->vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    CHECK_VK(vkCreateCommandPool(device_, &createInfo, VK_NULL_HANDLE, &commandPool))
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

void Renderer::recordCommandBuffer(VkCommandBuffer cb, const uint32_t imgIndex) const
{
    CHECK_VK(vkResetCommandBuffer(cb, 0))

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    CHECK_VK(vkBeginCommandBuffer(cb, &beginInfo))

    VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };

    VkRenderingAttachmentInfo renderingAttachmentInfo {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchainImageViews_[imgIndex],
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

    utils::vk::transitionImageLayout(cb,
                                     swapchainImages_[imgIndex],
                                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     VK_PIPELINE_STAGE_2_NONE,
                                     VK_ACCESS_NONE,
                                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
    );

    vkCmdBeginRendering(cb, &renderingInfo);

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

    VkDeviceSize vertOffset = 0;
    vkCmdBindVertexBuffers(cb, 0, 1, &vertBuffer_, &vertOffset);

    vkCmdDraw(cb, 3, 1, 0, 0);

    // Render ImGui draw data within the same rendering scope
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb, VK_NULL_HANDLE);

    vkCmdEndRendering(cb);

    utils::vk::transitionImageLayout(cb,
                                     swapchainImages_[imgIndex],
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                                     VK_PIPELINE_STAGE_2_NONE,
                                     VK_ACCESS_NONE
    );

    CHECK_VK(vkEndCommandBuffer(cb))
}

Slang::ComPtr<slang::ISession> Renderer::setupSlangSession(const Slang::ComPtr<slang::IGlobalSession>& globalSession) const
{
    slang::TargetDesc target_desc {
        .format = SLANG_SPIRV,
        .profile = globalSession->findProfile("spirv_1_4")
    };

    std::array<slang::CompilerOptionEntry, 1> options = {
        {
            slang::CompilerOptionName::EmitSpirvDirectly,
            {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
        }
    };

    slang::SessionDesc sessionDesc {
        .targets = &target_desc,
        .targetCount = 1,
        .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
        .compilerOptionEntries = options.data(),
        .compilerOptionEntryCount = static_cast<uint32_t>(options.size()),
    };

    Slang::ComPtr<slang::ISession> session;
    globalSession->createSession(sessionDesc, session.writeRef());

    return session;
}
} // spectra
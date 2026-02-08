//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Application.h"

#include <array>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "Utilities.h"
#include "Renderer.h"
#include "vk/Error.h"

namespace spectra {
Application::Application()
{
    pCtx_ = std::make_shared<vk::Context>();
    pCtx_->init();

    utils::vk::createTemporaryCommandPool(
        pCtx_->device, pCtx_->vkbDevice.get_queue_index(vkb::QueueType::graphics).value(), temporaryCmdPool_);

    createSwapchain();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    setupImGui();

    pRenderer_ = std::make_unique<Renderer>(pCtx_, vkbSwapchain_, swapchainImageViews_);
    pRenderer_->loadScene("scenes/SunglassesKhronos.glb");
}

Application::~Application()
{
    // Shutdown ImGui before destroying the descriptor pool it uses
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(pCtx_->device, imguiDescriptorPool_, nullptr);

    vkDestroyCommandPool(pCtx_->device, temporaryCmdPool_, nullptr);

    vkbSwapchain_.destroy_image_views(swapchainImageViews_);
    vkb::destroy_swapchain(vkbSwapchain_);

    pCtx_->deinit();
}

void Application::run()
{
    // Main loop
    while (!glfwWindowShouldClose(pCtx_->pWindow))
    {
        glfwPollEvents();

        // Build ImGui frame and UI
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplVulkan_NewFrame();
        ImGui::NewFrame();

        pRenderer_->render();
    }
    vkDeviceWaitIdle(pCtx_->device);

    pRenderer_->shutdown();
}

void Application::createSwapchain()
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
    utils::vk::beginOneTimeCommands(cmd, pCtx_->device, temporaryCmdPool_);

    // Set initial swapchain image layouts to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    for (int i = 0; i < vkbSwapchain_.image_count; i++)
    {
        utils::vk::transitionImageLayout(cmd,
            swapchainImages_[i],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_NONE,
            VK_ACCESS_NONE
        );
    }

    utils::vk::endOneTimeCommands(cmd, pCtx_->device, temporaryCmdPool_, pCtx_->graphicsQueue);
}

void Application::setupImGui()
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

} // spectra
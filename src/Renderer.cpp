//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Renderer.h"

#include <utility>

#include "Error.h"
#include "ShaderModule.h"

#include "triangle.vert.h"
#include "triangle.frag.h"

namespace spectra {
Renderer::Renderer(std::shared_ptr<vk::Context> context) : pCtx_(std::move(context))
{
    createGraphicsPipeline();
}

void Renderer::start()
{
    // Main loop
    while (!glfwWindowShouldClose(pCtx_->pWindow))
    {
        glfwPollEvents();

        render();
    }
}

void Renderer::render()
{

}

void Renderer::createGraphicsPipeline()
{
    // TODO: Need a way to destroy on exit. Handle that in the ShaderModule class.
    VkShaderModule triVert = vk::ShaderModule::createShaderModule(pCtx_->device, triangle_vert);
    VkShaderModule triFrag = vk::ShaderModule::createShaderModule(pCtx_->device, triangle_frag);

    if (triVert == VK_NULL_HANDLE || triFrag == VK_NULL_HANDLE)
    {
        std::cerr << "Failed to create shader modules\n";
    }
}
} // spectra
//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Renderer.h"

#include <utility>

#include "Error.h"

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

    shutdown();
}

void Renderer::render()
{

}

void Renderer::shutdown()
{
    pShaderTriangleVert_->destroy();
    pShaderTriangleFrag_->destroy();
}


void Renderer::createGraphicsPipeline()
{
    pShaderTriangleVert_ = std::make_unique<vk::ShaderModule>(pCtx_->device, triangle_vert);
    pShaderTriangleFrag_ = std::make_unique<vk::ShaderModule>(pCtx_->device, triangle_frag);

    if (pShaderTriangleVert_->value() == VK_NULL_HANDLE || pShaderTriangleFrag_->value() == VK_NULL_HANDLE)
    {
        std::cerr << "Failed to create shader modules\n";
    }
}
} // spectra
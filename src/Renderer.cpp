//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Renderer.h"

#include <utility>

namespace spectra {
Renderer::Renderer(std::shared_ptr<vk::Context> context) : pCtx_(std::move(context))
{
    // TODO: Init Vulkan stuff

}

void Renderer::draw()
{
    while (!glfwWindowShouldClose(pCtx_->pWindow))
    {
        glfwPollEvents();
    }
}
} // spectra
//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Renderer.h"

#include <utility>

#include "error.h"

namespace spectra {
Renderer::Renderer(std::shared_ptr<vk::Context> context) : pCtx_(std::move(context))
{
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
} // spectra
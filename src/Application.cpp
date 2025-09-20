//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Application.h"

#include "Renderer.h"

namespace spectra {
Application::Application()
{
    pVkCtx_ = std::make_shared<vk::Context>();
    pVkCtx_->init();
    pRenderer_ = std::make_unique<Renderer>(pVkCtx_);
}

Application::~Application()
{
    pVkCtx_->deinit();
}

void Application::run()
{
    pRenderer_->start();
}

} // spectra
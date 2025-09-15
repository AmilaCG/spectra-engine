//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Application.h"

#include "Renderer.h"

namespace spectra {
Application::Application()
{
    pVkContext_ = std::make_shared<vk::Context>();
    pVkContext_->init();
    pRenderer_ = std::make_unique<Renderer>(pVkContext_);
}

Application::~Application()
{
    pVkContext_->deinit();
}

void Application::run()
{
    pRenderer_->draw();
}

} // spectra
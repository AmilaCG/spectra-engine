//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Application.h"

#include "Renderer.h"

namespace vkpbr {
Application::Application()
{
    pRenderer_ = std::make_unique<Renderer>();
}

void Application::run()
{
    pRenderer_->draw(vkContext_.pWindow);
}

} // vkpbr
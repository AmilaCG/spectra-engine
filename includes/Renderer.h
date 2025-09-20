//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#ifndef RENDERER_H
#define RENDERER_H

#include <memory>
#include <GLFW/glfw3.h>

#include "Context.h"

namespace spectra {
class Renderer {
public:
    Renderer(std::shared_ptr<vk::Context> context);

    void start();

private:
    void render();
    void createGraphicsPipeline();

    std::shared_ptr<vk::Context> pCtx_;
};
} // spectra

#endif //RENDERER_H

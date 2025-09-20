//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#ifndef RENDERER_H
#define RENDERER_H

#include <memory>

#include "Context.h"
#include "ShaderModule.h"

namespace spectra {
class Renderer {
public:
    Renderer(std::shared_ptr<vk::Context> context);

    void start();

private:
    void render();
    void shutdown();
    void createGraphicsPipeline();

    std::shared_ptr<vk::Context>        pCtx_;

    std::unique_ptr<vk::ShaderModule>   pShaderTriangleVert_;
    std::unique_ptr<vk::ShaderModule>   pShaderTriangleFrag_;
};
} // spectra

#endif //RENDERER_H

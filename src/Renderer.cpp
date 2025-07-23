//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Renderer.h"

namespace vkpbr {
Renderer::Renderer(GLFWwindow* window)
{
    // TODO: Init Vulkan stuff

    draw(window);
}

void Renderer::draw(GLFWwindow* window)
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}
} // vkpbr
//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Renderer.h"

namespace spectra {
Renderer::Renderer()
{
    // TODO: Init Vulkan stuff

}

void Renderer::draw(GLFWwindow* window)
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}
} // spectra
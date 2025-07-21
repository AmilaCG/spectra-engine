//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Application.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vkpbr {
void Application::Init()
{
    InitWindow();
}

void Application::Run()
{
}

void Application::Close()
{
}

// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/00_Base_code.html#_integrating_glfw
void Application::InitWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Do not create an OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Disable window resizing for now
}
} // vkpbr
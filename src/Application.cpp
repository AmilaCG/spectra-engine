//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#include "Application.h"

#include "Context.h"
#include "Renderer.h"

namespace vkpbr {
Application::~Application()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Application::init()
{
    initWindow();

    vk::Context vkContext(m_window);
}

void Application::run()
{
    Renderer renderer(m_window);
}

// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/00_Base_code.html#_integrating_glfw
void Application::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Do not create an OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Disable window resizing for now

    m_window = glfwCreateWindow(1280, 720, "PBR Engine", nullptr, nullptr);
}
} // vkpbr
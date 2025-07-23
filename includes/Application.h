//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#ifndef APPLICATION_H
#define APPLICATION_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

namespace vkpbr {

class Application {
public:
    ~Application();

    void init();
    void run();

private:
    void initWindow();

private:
    GLFWwindow* m_window{};
};

} // vkpbr

#endif //APPLICATION_H

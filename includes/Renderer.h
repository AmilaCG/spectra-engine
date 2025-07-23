//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#ifndef RENDERER_H
#define RENDERER_H

#include <GLFW/glfw3.h>

namespace vkpbr {
class Renderer {
public:
    explicit Renderer(GLFWwindow* window);

private:
    void draw(GLFWwindow* window);
};
} // vkpbr

#endif //RENDERER_H

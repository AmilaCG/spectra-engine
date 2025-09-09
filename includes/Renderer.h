//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#ifndef RENDERER_H
#define RENDERER_H

#include <GLFW/glfw3.h>

namespace spectra {
class Renderer {
public:
    Renderer();

    void draw(GLFWwindow* window);
};
} // spectra

#endif //RENDERER_H

//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#ifndef APPLICATION_H
#define APPLICATION_H

#include <GLFW/glfw3.h>
#include <memory>
#include "Context.h"
#include "Renderer.h"

namespace spectra {

class Application {
public:
    Application();

    void init();
    void run();

private:
    void initWindow();

private:
    vk::Context vkContext_{};
    std::unique_ptr<Renderer> pRenderer_;
};

} // spectra

#endif //APPLICATION_H

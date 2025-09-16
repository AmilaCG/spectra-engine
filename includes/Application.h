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
    ~Application();

    void run();

private:
    std::shared_ptr<vk::Context> pVkContext_;
    std::unique_ptr<Renderer> pRenderer_;
};

} // spectra

#endif //APPLICATION_H

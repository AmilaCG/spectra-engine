//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include "Renderer.h"

namespace spectra {

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    std::unique_ptr<Renderer> pRenderer_;
};

} // spectra

#endif //APPLICATION_H

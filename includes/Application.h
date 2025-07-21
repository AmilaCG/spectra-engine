//
// Created by Amila Abeygunasekara on 7/20/2025.
//

#ifndef APPLICATION_H
#define APPLICATION_H

namespace vkpbr {

class Application {
public:
    virtual ~Application() = default;

    virtual void Init();
    virtual void Run();
    virtual void Close();

private:
    void InitWindow();
};

} // vkpbr

#endif //APPLICATION_H

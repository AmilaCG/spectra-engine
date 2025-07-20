cmake_minimum_required(VERSION 3.31)
project(vk_pbr_engine)

set(CMAKE_CXX_STANDARD 20)

include(dependencies/CMakeLists.txt)

include_directories(includes)

add_executable(vk_pbr_engine
        src/main.cpp
        src/Application.cpp
        src/Renderer.cpp
)

target_sources(vk_pbr_engine PRIVATE
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)

target_include_directories(vk_pbr_engine PRIVATE
        ${imgui_SOURCE_DIR}
)

target_compile_definitions(vk_pbr_engine PRIVATE STB_IMAGE_IMPLEMENTATION)

find_package(Vulkan REQUIRED)

target_link_libraries(vk_pbr_engine
        PRIVATE
        Vulkan::Vulkan
        glm
        glfw
        VulkanMemoryAllocator
        tinygltf
)

cmake_minimum_required(VERSION 3.31)
project(spectra)

set(CMAKE_CXX_STANDARD 20)

include(cmake/Dependencies.cmake)

find_library(Slang_LIBRARY NAMES slang slang-compiler HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/Application.cpp
        src/Renderer.cpp
        src/vulkan/Context.cpp
)

target_sources(${PROJECT_NAME} PRIVATE
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${imgui_SOURCE_DIR}
)

target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        GLFW_INCLUDE_VULKAN
)

find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Vulkan::Vulkan
        glm
        glfw
        VulkanMemoryAllocator
        tinygltf
        vk-bootstrap::vk-bootstrap
        ${Slang_LIBRARY}
)

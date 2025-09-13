cmake_minimum_required(VERSION 3.31)
project(spectra)

set(CMAKE_CXX_STANDARD 20)

include(dependencies/CMakeLists.txt)

include_directories(includes)

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
        STB_IMAGE_IMPLEMENTATION
        GLFW_INCLUDE_VULKAN
)

# Define VK_NO_PROTOTYPES to avoid including Vulkan prototypes
# This is necessary because we are using volk to load Vulkan functions
target_compile_definitions(volk PUBLIC VK_NO_PROTOTYPES)

find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Vulkan::Vulkan
        glm
        glfw
        VulkanMemoryAllocator
        tinygltf
        vk-bootstrap::vk-bootstrap
        volk
)

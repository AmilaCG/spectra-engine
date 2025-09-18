cmake_minimum_required(VERSION 3.31)
project(spectra)

set(CMAKE_CXX_STANDARD 20)

include(cmake/Dependencies.cmake)
include(cmake/third_party/CompilerGlslShader.cmake)

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

find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Vulkan::Vulkan
        glm
        glfw
        VulkanMemoryAllocator
        tinygltf
        vk-bootstrap::vk-bootstrap
)

# Compile shaders
file(GLOB SHADER_H_FILES "shaders/*.h")
file(GLOB SHADER_FILES "shaders/*.vert" "shaders/*.frag" "shaders/*.comp")

message(STATUS "Compiling shaders: ${SHADER_FILES}")

compile_glsl(
        "${SHADER_FILES}"
        "${CMAKE_BINARY_DIR}/generated_shaders"
        GENERATED_SHADER_HEADERS
)

# Add shader files to the project
source_group("Shaders" FILES ${SHADER_FILES} ${SHADER_H_FILES})
source_group("Shaders/Compiled" FILES ${GENERATED_SHADER_HEADERS})

# Add the output shader headers (target) directly to the executable
target_sources(${PROJECT_NAME} PRIVATE ${GENERATED_SHADER_HEADERS} ${SHADER_H_FILES})

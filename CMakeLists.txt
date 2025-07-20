cmake_minimum_required(VERSION 3.31)
project(vk_pbr_engine)

set(CMAKE_CXX_STANDARD 20)

include(cmake/Dependencies.cmake)

include_directories(includes)

add_executable(vk_pbr_engine
        main.cpp
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

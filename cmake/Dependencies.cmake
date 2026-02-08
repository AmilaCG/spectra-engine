cmake_minimum_required(VERSION 3.31)
include(FetchContent)

FetchContent_Declare(
        glm
        GIT_REPOSITORY  https://github.com/g-truc/glm
        GIT_TAG         1.0.1
        GIT_SHALLOW     TRUE
)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw
        GIT_TAG        3.3.8
        GIT_SHALLOW    TRUE
)
FetchContent_Declare(
        vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        GIT_TAG        v3.0.1
        GIT_SHALLOW    TRUE
)
FetchContent_Declare(
        tinygltf
        GIT_REPOSITORY https://github.com/syoyo/tinygltf
        GIT_TAG        v2.8.3
        GIT_SHALLOW    TRUE
)
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.92.1
        GIT_SHALLOW    TRUE
)
FetchContent_Declare(
        vk_bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
        GIT_TAG        v1.4.320
        GIT_SHALLOW    TRUE
)

FetchContent_Declare(
        slang
        GIT_REPOSITORY https://github.com/shader-slang/slang.git
        GIT_TAG        v2026.1.2
        GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(glm glfw vma imgui tinygltf vk_bootstrap slang)

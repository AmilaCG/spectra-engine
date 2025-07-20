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
        stb_image
        GIT_REPOSITORY https://github.com/nothings/stb
        GIT_TAG        f58f558c120e9b32c217290b80bad1a0729fbb2c
        GIT_SHALLOW    TRUE
)

FetchContent_Declare(
        tinygltf
        GIT_REPOSITORY https://github.com/syoyo/tinygltf
        GIT_TAG        v2.8.3
        GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(glm glfw vma stb_image tinygltf)

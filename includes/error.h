//
// Created by Amila Abeygunasekara on Mon 08/09/2025.
//

#ifndef SPECTRA_ERROR_H
#define SPECTRA_ERROR_H

#include <vulkan/vk_enum_string_helper.h>  // For string_VkResult

#define VK_CHECK(x)                                                            \
    do                                                                         \
    {                                                                          \
        VkResult res = x;                                                      \
        if (res)                                                               \
        {                                                                      \
            throw std::runtime_error("Vulkan error: " + string_VkResult(res)); \
        }                                                                      \
    } while (0)

#endif //SPECTRA_ERROR_H
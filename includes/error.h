//
// Created by Amila Abeygunasekara on Mon 08/09/2025.
//

#ifndef SPECTRA_ERROR_H
#define SPECTRA_ERROR_H

#include <stdexcept>
#include <string>
#include <vulkan/vk_enum_string_helper.h>

namespace spectra {

inline void checkVk(VkResult res)
{
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("Vulkan error: " + std::string(string_VkResult(res)));
    }
}
} // namespace spectra

#endif //SPECTRA_ERROR_H
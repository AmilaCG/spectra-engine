//
// Created by Amila Abeygunasekara on Mon 08/09/2025.
//

#ifndef SPECTRA_ERROR_H
#define SPECTRA_ERROR_H

#include <iostream>
#include <string>
#include <vulkan/vk_enum_string_helper.h>

namespace spectra {

inline void checkVk(VkResult res, const char* file, int line)
{
    if (res != VK_SUCCESS)
    {
        std::cerr << "Vulkan error: "
                  << std::string(string_VkResult(res))
                  << " at " << file << ": " << line
                  << std::endl;
        abort();
    }
}
#define CHECK_VK(x) checkVk((x), __FILE__, __LINE__);
} // namespace spectra

#endif //SPECTRA_ERROR_H
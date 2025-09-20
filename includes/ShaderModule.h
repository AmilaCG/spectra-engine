//
// Created by Amila Abeygunasekara on Sat 20/09/2025.
//

#ifndef SPECTRA_SHADERMODULE_H
#define SPECTRA_SHADERMODULE_H

#include <vulkan/vulkan.h>

#include "Error.h"
#include "Context.h"

namespace spectra::vk {
class ShaderModule {
public:
    template <size_t N>
    static VkShaderModule createShaderModule(VkDevice device, const uint32_t (&code)[N])
    {
        VkShaderModuleCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = sizeof(code[0]) * N,
            .pCode = code
        };

        VkShaderModule shaderModule = VK_NULL_HANDLE;
        CHECK_VK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule))

        return shaderModule;
    }
};

} // spectra::vk

#endif //SPECTRA_SHADERMODULE_H
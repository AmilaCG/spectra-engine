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
    ShaderModule() = default;

    template <size_t N>
    ShaderModule(VkDevice device, const uint32_t (&code)[N]) : device_(device)
    {
        VkShaderModuleCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = sizeof(code[0]) * N,
            .pCode = code
        };

        CHECK_VK(vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule_))
    }

    ~ShaderModule()
    {
        if (shaderModule_ != VK_NULL_HANDLE)
        {
            std::cerr << "Shader module is not destroyed on destruction!\n";
        }
    }

    void destroy()
    {
        vkDestroyShaderModule(device_, shaderModule_, nullptr);
        shaderModule_ = VK_NULL_HANDLE;
    }

    [[nodiscard]] VkShaderModule value() const
    {
        if (shaderModule_ == VK_NULL_HANDLE)
        {
            std::cerr << "Shader module is not initialized!\n";
            abort();
        }
        return shaderModule_;
    }

private:
    VkDevice        device_ = VK_NULL_HANDLE;
    VkShaderModule  shaderModule_ = VK_NULL_HANDLE;
};
} // spectra::vk

#endif //SPECTRA_SHADERMODULE_H
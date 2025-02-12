#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"

#include "Utils/Helpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

ShaderManager::ShaderManager(const VulkanContext& aVulkanContext)
    : vulkanContext{aVulkanContext}
{}

// TODO: Deduct shader type from SPIR-V bytecode
ShaderModule ShaderManager::CreateShaderModule(const std::string& filePath, const ShaderType shaderType) const
{    
    const std::vector<char> shaderCode = Helpers::ReadFile(filePath);
    
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule shaderModule;
    const VkResult result = vkCreateShaderModule(vulkanContext.GetDevice().GetVkDevice(), &createInfo, nullptr, &shaderModule);
    Assert(result == VK_SUCCESS);

    return ShaderModule(shaderModule, shaderType, vulkanContext);
}
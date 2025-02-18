#include "Engine/Render/Resources/Shaders/ShaderManager.hpp"

#include "Utils/Helpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

ShaderManager::ShaderManager(const VulkanContext& aVulkanContext)
    : vulkanContext{aVulkanContext}
{}

ShaderModule ShaderManager::CreateShaderModule(const FilePath& path, const ShaderType shaderType) const
{
    const std::vector<char> glslCode = FileSystem::ReadFile(path);

    const std::vector<uint32_t> spirvCode = ShaderCompiler::Compile(std::string_view(glslCode.data(), glslCode.size()), 
        shaderType, FilePath("~/Shaders"));

    if (spirvCode.empty())
    {
        return { VK_NULL_HANDLE, shaderType, vulkanContext };
    }
    
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
    createInfo.pCode = spirvCode.data();

    VkShaderModule shaderModule;
    const VkResult result = vkCreateShaderModule(vulkanContext.GetDevice().GetVkDevice(), &createInfo, nullptr, &shaderModule);
    Assert(result == VK_SUCCESS);

    return { shaderModule, shaderType, vulkanContext };
}

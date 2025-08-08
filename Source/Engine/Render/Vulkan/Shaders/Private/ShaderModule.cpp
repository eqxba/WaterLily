#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace ShaderModuleDetails {}

ShaderModule::ShaderModule(const VkShaderModule aShaderModule, ShaderReflection aReflection,
    const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , shaderModule{ aShaderModule }
    , reflection{ std::move(aReflection) }
{}

ShaderModule::~ShaderModule()
{
    if (shaderModule != VK_NULL_HANDLE)
    { 
        vkDestroyShaderModule(vulkanContext->GetDevice(), shaderModule, nullptr);
    }    
}

ShaderModule::ShaderModule(ShaderModule&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , shaderModule{ other.shaderModule }
    , reflection{ std::move(other.reflection) }
{
    other.shaderModule = VK_NULL_HANDLE;
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept
{
    if (this != &other)
    {
        std::swap(vulkanContext, other.vulkanContext);
        std::swap(shaderModule, other.shaderModule);
        std::swap(reflection, other.reflection);
    }
    return *this;
}

bool ShaderModule::IsValid() const
{
    return shaderModule != VK_NULL_HANDLE;
}

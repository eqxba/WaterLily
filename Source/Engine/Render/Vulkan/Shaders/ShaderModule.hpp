#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetReflection.hpp"

class VulkanContext;

using ShaderDefine = std::pair<std::string_view, std::string_view>;

struct SpecializationConstantReflection
{
    uint32_t costantId = 0;
    std::string name;
};

struct ShaderReflection
{
    VkShaderStageFlagBits shaderStage;
    std::vector<DescriptorSetReflection> descriptorSets;
    std::vector<SpecializationConstantReflection> specializationConstants;
};

class ShaderModule;

struct ShaderInstance
{
    const ShaderModule* shaderModule = nullptr;
    std::vector<VkSpecializationMapEntry> specializationMapEntries;
    std::vector<std::byte> specializationData;
    VkSpecializationInfo specializationInfo;
};

class ShaderModule
{
public:
    ShaderModule(const VkShaderModule shaderModule, ShaderReflection reflection, const VulkanContext& aVulkanContext);
    ~ShaderModule();

    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;

    ShaderModule(ShaderModule&& other) noexcept;
    ShaderModule& operator=(ShaderModule&& other) noexcept;

    bool IsValid() const;
    
    const ShaderReflection& GetReflection() const
    {
        return reflection;
    }
    
    operator VkShaderModule() const
    {
        return shaderModule;
    }

private:
    const VulkanContext* vulkanContext = nullptr;

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    ShaderReflection reflection;
};

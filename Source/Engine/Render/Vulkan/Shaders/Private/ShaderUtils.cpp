#include "Engine/Render/Vulkan/Shaders/ShaderUtils.hpp"

#include <spirv_reflect.h>

namespace ShaderUtilsDetails
{
    static VkShaderStageFlagBits GetShaderStage(const SpvReflectShaderStageFlagBits shaderStage)
    {
        switch (shaderStage)
        {
            case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: return VK_SHADER_STAGE_VERTEX_BIT;
            case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT: return VK_SHADER_STAGE_GEOMETRY_BIT;
            case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: return VK_SHADER_STAGE_FRAGMENT_BIT;
            case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT: return VK_SHADER_STAGE_COMPUTE_BIT;
            case SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT: return VK_SHADER_STAGE_TASK_BIT_EXT;
            case SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT: return VK_SHADER_STAGE_MESH_BIT_EXT;
            case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR: return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR: return VK_SHADER_STAGE_MISS_BIT_KHR;
            case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR: return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR: return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
        }
        
        Assert(false);
        return {};
    }

    static VkDescriptorType GetDescriptorType(SpvReflectDescriptorType descriptorType)
    {
        switch (descriptorType)
        {
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        }
        
        Assert(false);
        return {};
    }

    static std::vector<BindingReflection> GetBindindingsReflection(
        const SpvReflectDescriptorSet& setReflection, const VkShaderStageFlagBits shaderStage)
    {
        std::vector<BindingReflection> bindingsReflection;
        bindingsReflection.reserve(setReflection.binding_count);
        
        for (const SpvReflectDescriptorBinding* binding : std::span(setReflection.bindings, setReflection.binding_count))
        {
            bindingsReflection.push_back({
                .index = binding->binding,
                .names = { *binding->name != 0 ? binding->name : binding->type_description->type_name },
                .type = GetDescriptorType(binding->descriptor_type),
                .shaderStages = static_cast<VkShaderStageFlags>(shaderStage), });
        }
        
        return bindingsReflection;
    }

    static std::vector<DescriptorSetReflection> GetDescriptorSetReflections(
        const spv_reflect::ShaderModule& shaderModule, const VkShaderStageFlagBits shaderStage)
    {
        uint32_t descriptorSetsCount;
        SpvReflectResult result = shaderModule.EnumerateDescriptorSets(&descriptorSetsCount, nullptr);
        Assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectDescriptorSet*> descriptorSets(descriptorSetsCount);
        result = shaderModule.EnumerateDescriptorSets(&descriptorSetsCount, descriptorSets.data());
        Assert(result == SPV_REFLECT_RESULT_SUCCESS);
        
        std::vector<DescriptorSetReflection> setsReflection;
        setsReflection.reserve(descriptorSetsCount);
        
        for (const SpvReflectDescriptorSet* descriptorSet : descriptorSets)
        {
            setsReflection.emplace_back(descriptorSet->set, GetBindindingsReflection(*descriptorSet, shaderStage));
        }
        
        return setsReflection;
    }

    static void MergeBindings(DescriptorSetReflection& setReflection, const std::vector<BindingReflection>& bindings)
    {
        for (const BindingReflection& newBinding : bindings)
        {
            const auto bindingIt = std::ranges::find_if(setReflection.bindings, [&](const BindingReflection& existingBinding) {
                return newBinding.index == existingBinding.index; });

            if (bindingIt == setReflection.bindings.end())
            {
                setReflection.bindings.push_back(newBinding);
            }
            else
            {
                BindingReflection& existingBinding = *bindingIt;

                Assert(existingBinding.type == newBinding.type);

                existingBinding.shaderStages |= newBinding.shaderStages;

                for (const std::string& newName : newBinding.names)
                {
                    if (!std::ranges::contains(existingBinding.names, newName))
                    {
                        existingBinding.names.push_back(newName);
                    }
                }
            }
        }
    }

    static std::unordered_map<std::string, VkPushConstantRange> GetPushConstantReflections(
        const spv_reflect::ShaderModule& shaderModule, const VkShaderStageFlagBits shaderStage)
    {
        std::unordered_map<std::string, VkPushConstantRange> reflections;
        
        uint32_t pushConstantCount;
        bool result = shaderModule.EnumeratePushConstantBlocks(&pushConstantCount, nullptr);
        Assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
        result = shaderModule.EnumeratePushConstantBlocks(&pushConstantCount, pushConstants.data());
        Assert(result == SPV_REFLECT_RESULT_SUCCESS);
        
        for (const auto pushConstant : pushConstants)
        {
            for (const SpvReflectBlockVariable& member : std::span(pushConstant->members, pushConstant->member_count))
            {
                reflections.emplace(member.name, VkPushConstantRange(shaderStage, member.offset, member.size));
            }
        }
       
        return reflections;
    }

    static std::vector<SpecializationConstantReflection> GetSpecializationConstantReflections(
        const spv_reflect::ShaderModule& shaderModule)
    {
        uint32_t specializationConstantCount;
        SpvReflectResult result = shaderModule.EnumerateSpecializationConstants(&specializationConstantCount, nullptr);
        Assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectSpecializationConstant*> specializationConstants(specializationConstantCount);
        result = shaderModule.EnumerateSpecializationConstants(&specializationConstantCount, specializationConstants.data());
        Assert(result == SPV_REFLECT_RESULT_SUCCESS);
        
        std::vector<SpecializationConstantReflection> reflection;
        reflection.reserve(specializationConstantCount);
        
        for (const auto& constant : specializationConstants)
        {
            reflection.emplace_back(constant->constant_id, constant->name);
        }
        
        return reflection;
    }

    static std::pair<std::vector<VkSpecializationMapEntry>, std::vector<std::byte>> CreateSpecializationData(
        const ShaderModule& shaderModule, const std::vector<SpecializationConstant>& specializationConstants)
    {
        std::vector<VkSpecializationMapEntry> mapEntries;
        std::vector<std::byte> data;
        
        for (const SpecializationConstantReflection& reflection : shaderModule.GetReflection().specializationConstants)
        {
            const auto& it = std::ranges::find_if(specializationConstants, [&](const SpecializationConstant& constant){
                return constant.name == reflection.name;
            });
            
            if (it != specializationConstants.end())
            {
                const auto offset = static_cast<uint32_t>(data.size());
                
                const auto& visitPred = [&](const auto& value){
                    using T = std::decay_t<decltype(value)>;
                    const size_t size = sizeof(T);
                    
                    data.resize(data.size() + size);
                    std::memcpy(data.data() + offset, &value, size);
                    
                    mapEntries.emplace_back(reflection.costantId, offset, size);
                };
                
                std::visit(visitPred, it->value);
            }
        }
        
        return { std::move(mapEntries), std::move(data) };
    }
}

void ShaderUtils::InsertDefines(std::string& glslCode, const std::span<const ShaderDefine> shaderDefines)
{
    if (shaderDefines.empty())
    {
        return;
    }
    
    const size_t versionPos = glslCode.find("#version");
    
    if (versionPos == std::string::npos)
    {
        return;
    }
    
    size_t versionLineEndPos = glslCode.find('\n', versionPos);
    
    if (versionLineEndPos == std::string::npos)
    {
        versionLineEndPos = glslCode.length();
    }

    std::string definesBlock = "\n";

    for (const auto& define : shaderDefines)
    {
        definesBlock += std::format("#define {} {}\n", define.first, define.second);
    }

    glslCode.insert(versionLineEndPos + 1, definesBlock);
}

ShaderReflection ShaderUtils::GenerateReflection(const std::span<const uint32_t> spirvCode,
    const VkShaderStageFlagBits shaderStage)
{
    using namespace ShaderUtilsDetails;
    
    const auto shaderModule = spv_reflect::ShaderModule(spirvCode.size_bytes(), spirvCode.data());

    Assert(GetShaderStage(shaderModule.GetShaderStage()) == shaderStage);
    
    return {
        .shaderStage = shaderStage,
        .descriptorSets = GetDescriptorSetReflections(shaderModule, shaderStage),
        .pushConstants = GetPushConstantReflections(shaderModule, shaderStage),
        .specializationConstants = GetSpecializationConstantReflections(shaderModule) };
}

std::vector<DescriptorSetReflection> ShaderUtils::MergeDescriptorSetReflections(const std::span<const ShaderModule> shaderModules)
{
    std::vector<DescriptorSetReflection> resultReflections;
    
    for (const ShaderModule& shaderModule : shaderModules)
    {
        const ShaderReflection& shaderReflection = shaderModule.GetReflection();
        
        for (const DescriptorSetReflection& shaderSetReflection : shaderReflection.descriptorSets)
        {
            auto setIt = std::ranges::find_if(resultReflections, [&](const auto& reflection){
                return reflection.index == shaderSetReflection.index; });
            
            const bool found = setIt != resultReflections.end();
            
            if (!found)
            {
                resultReflections.push_back({ .index = shaderSetReflection.index });
            }
            
            DescriptorSetReflection& setReflection = found ? *setIt : resultReflections.back();
            
            ShaderUtilsDetails::MergeBindings(setReflection, shaderSetReflection.bindings);
        }
    }
    
    return resultReflections;
}

std::unordered_map<std::string, VkPushConstantRange> ShaderUtils::MergePushConstantReflections(const std::span<const ShaderModule> shaderModules)
{
    std::unordered_map<std::string, VkPushConstantRange> resultReflections;
    
    for (const ShaderModule& shaderModule : shaderModules)
    {
        const ShaderReflection& shaderReflection = shaderModule.GetReflection();
        
        for (const auto& [name, range] : shaderReflection.pushConstants)
        {
            if (resultReflections.contains(name))
            {
                VkPushConstantRange& existingRange = resultReflections[name];
                Assert(existingRange.size == range.size);
                Assert(existingRange.offset == range.offset);
                
                existingRange.stageFlags |= range.stageFlags;
            }
            else
            {
                resultReflections.emplace(name, range);
            }
        }
    }
    
    return resultReflections;
}

ShaderInstance ShaderUtils::CreateShaderInstance(const ShaderModule& shaderModule, const std::vector<SpecializationConstant>& specializationConstants)
{
    using namespace ShaderUtilsDetails;
    
    ShaderInstance shaderInstance = { .shaderModule = &shaderModule };
    
    if (auto [entries, data] = CreateSpecializationData(shaderModule, specializationConstants); !entries.empty())
    {
        shaderInstance.specializationInfo = { static_cast<uint32_t>(entries.size()), entries.data(), data.size(), data.data() };
        shaderInstance.specializationMapEntries = std::move(entries);
        shaderInstance.specializationData = std::move(data);
    }
    
    return shaderInstance;
}

VkPipelineShaderStageCreateInfo ShaderUtils::GetShaderStageCreateInfo(const ShaderInstance& shaderInstance)
{
    Assert(shaderInstance.shaderModule);
    
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = shaderInstance.shaderModule->GetReflection().shaderStage;
    shaderStageCreateInfo.module = *shaderInstance.shaderModule;
    shaderStageCreateInfo.pName = "main"; // Let's use "main" entry point by default
    shaderStageCreateInfo.pSpecializationInfo = shaderInstance.specializationMapEntries.empty() ? nullptr : &shaderInstance.specializationInfo;

    return shaderStageCreateInfo;
}

std::vector<VkPushConstantRange> ShaderUtils::GetPushConstantRanges(const std::unordered_map<std::string, VkPushConstantRange>& reflectionRanges)
{
    std::vector<VkPushConstantRange> resultRanges;
    
    for (const auto& [name, range] : reflectionRanges)
    {
        auto it = std::ranges::find_if(resultRanges, [&](const VkPushConstantRange& pushConstantRange){
            return pushConstantRange.stageFlags == range.stageFlags;
        });

        if (it != resultRanges.end())
        {
            if (it->offset < range.offset)
            {
                Assert(it->offset + it->size <= range.offset);
                it->size = range.offset + range.size - it->offset;
            }
            else
            {
                Assert(range.offset + range.size <= it->offset);
                it->size = it->offset + it->size - range.offset;
                it->offset = range.offset;
            }
        }
        else
        {
            resultRanges.push_back(range);
        }
    }
    
    return resultRanges;
}

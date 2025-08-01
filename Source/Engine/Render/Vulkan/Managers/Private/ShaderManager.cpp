#include "Engine/Render/Vulkan/Managers/ShaderManager.hpp"

#include <format>

DISABLE_WARNINGS_BEGIN
#include <SPIRV/GlslangToSpv.h>
DISABLE_WARNINGS_END

#include "Utils/Helpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderUtils.hpp"

namespace ShaderManagerDetails
{
    constexpr std::string_view shadersDir = "~/Shaders";
    constexpr std::string_view compiledShadersDir = "~/Shaders/Compiled";
    constexpr std::string_view compiledFileExtension = ".spv";

    static FilePath CreateCompiledShaderPath(const FilePath& path)
    {
        std::string relativeToShadersDir = path.GetRelativeTo(FilePath(shadersDir));
        relativeToShadersDir.append(compiledFileExtension);
        
        return FilePath(compiledShadersDir) / relativeToShadersDir;
    }
}

ShaderManager::ShaderManager(const VulkanContext& aVulkanContext)
    : vulkanContext{aVulkanContext}
{}

ShaderModule ShaderManager::CreateShaderModule(const FilePath& path, const VkShaderStageFlagBits shaderStage,
    const std::vector<ShaderDefine>& shaderDefines, bool useCacheOnFailure /* = true */) const
{
    using namespace ShaderManagerDetails;
    
    ScopeTimer timer(path.GetFileNameWithExtension());
    
    Assert(path.Exists());
    
    // Not very optimal but I don't care much so...
    const std::vector<char> glslCodeVector = FileSystem::ReadFile(path);
    auto glslCode = std::string(glslCodeVector.begin(), glslCodeVector.end());
    
    ShaderUtils::InsertDefines(glslCode, shaderDefines);
    
    const std::vector<uint32_t> spirv = ShaderCompiler::Compile(glslCode, shaderStage, FilePath(shadersDir));
    
    const FilePath compiledShaderPath = CreateCompiledShaderPath(path);
    
    // Create shader module on success
    if (!spirv.empty())
    {
        FileSystem::CreateDirectories(compiledShaderPath);
        glslang::OutputSpvBin(spirv, compiledShaderPath.GetAbsolute().c_str());
        
        return CreateShaderModule(spirv, shaderStage);
    }
    
    // Try to load shader module from cache if allowed
    if (useCacheOnFailure && compiledShaderPath.Exists())
    {
        const std::vector<char> cachedSpirv = FileSystem::ReadFile(compiledShaderPath);
        
        if (!cachedSpirv.empty())
        {
            const std::span spirvSpan(reinterpret_cast<const uint32_t*>(cachedSpirv.data()), cachedSpirv.size() / sizeof(uint32_t));
            
            return CreateShaderModule(spirvSpan, shaderStage);
        }
    }
    
    return { VK_NULL_HANDLE, {}, vulkanContext };
}

ShaderModule ShaderManager::CreateShaderModule(const std::span<const uint32_t> spirvCode, const VkShaderStageFlagBits shaderStage) const
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
    createInfo.pCode = spirvCode.data();

    VkShaderModule shaderModule;
    const VkResult result = vkCreateShaderModule(vulkanContext.GetDevice(), &createInfo, nullptr, &shaderModule);
    Assert(result == VK_SUCCESS);
    
    return { shaderModule, ShaderUtils::GenerateReflection(spirvCode, shaderStage), vulkanContext };
}

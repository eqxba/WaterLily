#include "Engine/Render/Vulkan/Managers/ShaderManager.hpp"

DISABLE_WARNINGS_BEGIN
#include <SPIRV/GlslangToSpv.h>
DISABLE_WARNINGS_END

#include "Utils/Helpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

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

ShaderModule ShaderManager::CreateShaderModule(const FilePath& path, const ShaderType shaderType,
    bool useCacheOnFailure /* = true */) const
{
    using namespace ShaderManagerDetails;
    
    ScopeTimer timer(path.GetFileNameWithExtension());
    
    Assert(path.Exists());
    
    const std::vector<char> glslCode = FileSystem::ReadFile(path);
    
    const std::vector<uint32_t> spirv = ShaderCompiler::Compile(std::string_view(glslCode.data(), glslCode.size()),
        shaderType, FilePath(shadersDir));
    
    const FilePath compiledShaderPath = CreateCompiledShaderPath(path);
    
    // Create shader module on success
    if (!spirv.empty())
    {
        FileSystem::CreateDirectories(compiledShaderPath);
        glslang::OutputSpvBin(spirv, compiledShaderPath.GetAbsolute().c_str());
        
        return CreateShaderModule(spirv, shaderType);
    }
    
    // Try to load shader module from cache if allowed
    if (useCacheOnFailure && compiledShaderPath.Exists())
    {
        const std::vector<char> cachedSpirv = FileSystem::ReadFile(compiledShaderPath);
        
        if (!cachedSpirv.empty())
        {
            const std::span spirvSpan(reinterpret_cast<const uint32_t*>(cachedSpirv.data()), cachedSpirv.size() / sizeof(uint32_t));
            
            return CreateShaderModule(spirvSpan, shaderType);
        }
    }
    
    return { VK_NULL_HANDLE, shaderType, vulkanContext };
}

ShaderModule ShaderManager::CreateShaderModule(const std::span<const uint32_t> spirvCode, const ShaderType shaderType) const
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
    createInfo.pCode = spirvCode.data();

    VkShaderModule shaderModule;
    const VkResult result = vkCreateShaderModule(vulkanContext.GetDevice(), &createInfo, nullptr, &shaderModule);
    Assert(result == VK_SUCCESS);
    
    return { shaderModule, shaderType, vulkanContext };
}

#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"

DISABLE_WARNINGS_BEGIN
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
DISABLE_WARNINGS_END

#include "Engine/FileSystem/FileSystem.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

namespace ShaderCompilerDetails
{
    static bool initialized = false;

    static std::unordered_map<VkShaderStageFlagBits, EShLanguage> glslangStages = {
        { VK_SHADER_STAGE_VERTEX_BIT, EShLangVertex },
        { VK_SHADER_STAGE_FRAGMENT_BIT, EShLangFragment },
        { VK_SHADER_STAGE_COMPUTE_BIT, EShLangCompute },
        { VK_SHADER_STAGE_TASK_BIT_EXT, EShLangTask },
        { VK_SHADER_STAGE_MESH_BIT_EXT, EShLangMesh },
    };
}

ShaderCompiler::ShaderCompiler()
{
    if (!ShaderCompilerDetails::initialized)
    {
        glslang::InitializeProcess();
        ShaderCompilerDetails::initialized = true;
    }
}

ShaderCompiler::~ShaderCompiler()
{
    if (ShaderCompilerDetails::initialized)
    {
        glslang::FinalizeProcess();
        ShaderCompilerDetails::initialized = false;
    }
}

std::vector<uint32_t> ShaderCompiler::Compile(const std::string_view glslCode, const VkShaderStageFlagBits shaderStage,
    const FilePath& includeDir)
{
    using namespace ShaderCompilerDetails;
    
    Assert(initialized);
    Assert(!glslCode.empty());
    
    static_assert(VulkanConfig::apiVersion == VK_API_VERSION_1_3); // Don't forget to change EShTargetClientVersion :)

    Assert(glslangStages.contains(shaderStage));
    const EShLanguage stage = glslangStages.find(shaderStage)->second;

    const char* code = glslCode.data();
    const auto length = static_cast<int>(glslCode.size());

    glslang::TShader shader(stage);

    shader.setStringsWithLengths(&code, &length, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

    DirStackFileIncluder includer;

    Assert(includeDir.IsDirectory());
    includer.pushExternalLocalDirectory(includeDir.GetAbsolute());

    const TBuiltInResource* defaultResources = GetDefaultResources();
    constexpr auto defaultMessages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    bool result = shader.parse(defaultResources, 100, false, defaultMessages, includer);

    if (!result)
    {
        LogE << "Failed to parse shader:\n" << shader.getInfoLog() << shader.getInfoDebugLog() << "\n";
        return {};
    }

    glslang::TProgram program;
    program.addShader(&shader);

    result = program.link(defaultMessages);

    if (!result)
    {
        LogE << "Failed to link shader:\n" << shader.getInfoLog() << shader.getInfoDebugLog() << "\n";
        return {};
    }
    
    std::vector<uint32_t> spirv;
    spv::SpvBuildLogger logger;
    
    GlslangToSpv(*program.getIntermediate(stage), spirv, &logger);

    if (const std::string messages = logger.getAllMessages(); !messages.empty())
    {
        LogI << "Spirv compiler messages:\n" << messages;
    }

    return spirv;
}

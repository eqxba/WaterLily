#include "Engine/Render/RenderStages/RenderStage.hpp"

RenderStage::RenderStage(const VulkanContext& aVulkanContext, RenderContext& aRenderContext)
    : vulkanContext{ &aVulkanContext }
    , renderContext{ &aRenderContext }
{}

RenderStage::~RenderStage()
{}

void RenderStage::Prepare(const Scene& scene)
{}

void RenderStage::Execute(const Frame& frame)
{}

void RenderStage::RecreatePipelinesAndDescriptors()
{}

void RenderStage::OnSceneClose()
{}

bool RenderStage::TryReloadShaders()
{
    reloadedShaders = CompileShadersImpl();
    return std::ranges::all_of(reloadedShaders, [&](const auto& entry){ return entry.second.IsValid(); });
}

void RenderStage::ApplyReloadedShaders()
{
    shaders = std::move(reloadedShaders);
    RecreatePipelinesAndDescriptors();
}

void RenderStage::CompileShaders()
{
    shaders = CompileShadersImpl();
}

void RenderStage::AddShaderInfo(ShaderInfo shaderInfo)
{
    shaderInfos.push_back(std::move(shaderInfo));
}

const ShaderModule* RenderStage::GetShader(const std::string_view path) const
{
    if (const auto it = shaders.find(path); it != shaders.end())
    {
        return &it->second;
    }
    
    return nullptr;
}

std::unordered_map<std::string_view, ShaderModule> RenderStage::CompileShadersImpl()
{
    std::unordered_map<std::string_view, ShaderModule> compiledShaders;
    
    const ShaderManager& shaderManager = vulkanContext->GetShaderManager();
    
    for (const ShaderInfo& info : shaderInfos)
    {
        const auto defines = info.definesGetter ? info.definesGetter() : std::span<const ShaderDefine>();
        ShaderModule shader = shaderManager.CreateShaderModule(FilePath(info.path), info.shaderStage, defines);
        compiledShaders.emplace(info.path, std::move(shader));
    }
    
    return compiledShaders;
}

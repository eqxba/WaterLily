#include "Engine/Render/RenderStages/RenderStage.hpp"

RenderStage::RenderStage(const VulkanContext& aVulkanContext, const RenderContext& aRenderContext)
    : vulkanContext{ &aVulkanContext }
    , renderContext{ &aRenderContext }
{}

RenderStage::~RenderStage()
{}

void RenderStage::CreateRenderTargetDependentResources()
{}

void RenderStage::DestroyRenderTargetDependentResources()
{}

void RenderStage::OnSceneOpen(const Scene& scene)
{}

void RenderStage::OnSceneClose()
{}

void RenderStage::Execute(const Frame& frame)
{}

void RenderStage::RebuildDescriptors()
{}

bool RenderStage::TryRebuildPipelines()
{
    std::ranges::transform(pipelineData, std::back_inserter(rebuiltPipelines), [&](const auto& pair) { return pair.second(); });
    
    if (std::ranges::all_of(rebuiltPipelines, &Pipeline::IsValid))
    {
        return true;
    }
    
    rebuiltPipelines.clear();
    
    return false;
}

void RenderStage::ApplyRebuiltPipelines()
{
    Assert(!rebuiltPipelines.empty() && rebuiltPipelines.size() == pipelineData.size());
    
    for (size_t i = 0; i < rebuiltPipelines.size(); ++i)
    {
        *pipelineData[i].first = std::move(rebuiltPipelines[i]);
    }
    
    RebuildDescriptors();
    
    rebuiltPipelines.clear();
}

void RenderStage::AddPipeline(Pipeline& pipelineReference, PipelineBuildFunction buildFunction)
{
    pipelineData.emplace_back(&pipelineReference, buildFunction);
    
    pipelineReference = buildFunction();
    Assert(pipelineReference.IsValid());
}

ShaderModule RenderStage::GetShader(const std::string_view path, const VkShaderStageFlagBits shaderStage,
    const std::span<std::string_view> runtimeDefines, const std::span<const ShaderDefine> aDefines) const
{
    const ShaderManager& shaderManager = vulkanContext->GetShaderManager();
    
    std::vector<ShaderDefine> defines(aDefines.begin(), aDefines.end());
    
    for (const auto& runtimDefine : runtimeDefines)
    {
        defines.emplace_back(runtimDefine, renderContext->runtimeDefineGetters.at(runtimDefine)());
    }
    
    std::ranges::sort(defines, [](auto &lhs, auto &rhs) { return lhs.first < rhs.first; });
    
    return shaderManager.CreateShaderModule(FilePath(path), shaderStage, defines);
}

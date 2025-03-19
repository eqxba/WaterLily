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

void RenderStage::RecreateFramebuffers()
{}

void RenderStage::TryReloadShaders()
{}

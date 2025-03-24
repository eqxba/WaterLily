#include "Engine/Render/RenderOptions.hpp"

#include "Engine/EventSystem.hpp"

namespace RenderOptionsDetails
{
    static std::unique_ptr<RenderOptions> renderOptions;
}

void RenderOptions::Initialize(const VulkanContext& vulkanContext, EventSystem& eventSystem)
{
    RenderOptionsDetails::renderOptions = std::make_unique<RenderOptions>(vulkanContext, eventSystem);
}

void RenderOptions::Deinitialize()
{
    RenderOptionsDetails::renderOptions.reset();
}

RenderOptions& RenderOptions::Get()
{
    Assert(RenderOptionsDetails::renderOptions);
    return *RenderOptionsDetails::renderOptions;
}

RenderOptions::RenderOptions(const VulkanContext& aVulkanContext, EventSystem& aEventSystem)
    : vulkanContext{ &aVulkanContext }
    , eventSystem{ &aEventSystem }
{
    // TODO: We can't init this to eMesh in class initializer as
    // it won't be verified on construction - that's why we're setting it here
    // and it will do the check inside the setter, better approach - clamp on load
    SetGraphicsPipelineType(GraphicsPipelineType::eMesh);
    
    eventSystem->Subscribe<ES::KeyInput>(this, &RenderOptions::OnKeyInput);
}

RenderOptions::~RenderOptions()
{
    eventSystem->UnsubscribeAll(this);
}

bool RenderOptions::IsGraphicsPipelineTypeSupported(GraphicsPipelineType aGraphicsPipelineType) const
{
    if (aGraphicsPipelineType == GraphicsPipelineType::eMesh)
    {
        return vulkanContext->GetDevice().GetProperties().meshShadersSupported;
    }
    
    return true;
}

RendererType RenderOptions::GetRendererType() const
{
    return rendererType;
}

void RenderOptions::SetRendererType(const RendererType aRendererType)
{
    rendererType = aRendererType;
}

GraphicsPipelineType RenderOptions::GetGraphicsPipelineType() const
{
    return graphicsPipelineType;
}

void RenderOptions::SetGraphicsPipelineType(const GraphicsPipelineType aGraphicsPipelineType)
{
    if (!IsGraphicsPipelineTypeSupported(aGraphicsPipelineType))
    {
        return;
    }
    
    graphicsPipelineType = aGraphicsPipelineType;
}

bool RenderOptions::GetUseLod() const
{
    return useLod;
}

void RenderOptions::SetUseLod(const bool aUseLod)
{
    useLod = aUseLod;
}

void RenderOptions::OnKeyInput(const ES::KeyInput& event)
{
    if (event.key == Key::eF1 && event.action == KeyAction::ePress)
    {
        SetGraphicsPipelineType(GraphicsPipelineType::eMesh);
    }
    
    if (event.key == Key::eF2 && event.action == KeyAction::ePress)
    {
        SetGraphicsPipelineType(GraphicsPipelineType::eVertex);
    }

    if (event.key == Key::eL && event.action == KeyAction::ePress)
    {
        SetUseLod(!useLod);
    }
}

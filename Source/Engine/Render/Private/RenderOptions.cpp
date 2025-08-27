#include "Engine/Render/RenderOptions.hpp"

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
        SetUseLods(!_UseLods);
    }
    
    if (event.key == Key::eSemicolon && event.action == KeyAction::ePress)
    {
        SetVisualizeLods(!_VisualizeLods);
    }

    if (event.key == Key::eF && event.action == KeyAction::ePress)
    {
        SetFreezeCamera(!_FreezeCamera);
    }
}

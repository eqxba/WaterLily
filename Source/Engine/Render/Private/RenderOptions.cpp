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
    // TODO: Loading from a config file, but now let's keep this initialization here, all setters here
    // do runtime clamping after vulkan initialization
    SetGraphicsPipelineType(GraphicsPipelineType::eMesh);
    SetMsaaSampleCount(vulkanContext->GetDevice().GetProperties().maxSampleCount);
    
    eventSystem->Subscribe<ES::KeyInput>(this, &RenderOptions::OnKeyInput);
}

RenderOptions::~RenderOptions()
{
    eventSystem->UnsubscribeAll(this);
}

bool RenderOptions::IsGraphicsPipelineTypeSupported(const GraphicsPipelineType graphicsPipelineType) const
{
    if (graphicsPipelineType == GraphicsPipelineType::eMesh)
    {
        return vulkanContext->GetDevice().GetProperties().meshShadersSupported;
    }
    
    return true;
}

bool RenderOptions::IsMsaaSampleCountSupported(const VkSampleCountFlagBits sampleCount) const
{
    return std::ranges::contains(OptionValues::msaaSampleCounts, sampleCount) &&
        sampleCount <= vulkanContext->GetDevice().GetProperties().maxSampleCount;
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
    
    if (event.key == Key::eT && event.action == KeyAction::ePress)
    {
        SetTestBool(!_TestBool);
    }
}

#include "Engine/Render/Ui/SettingsWidget.hpp"

#include "Engine/EventSystem.hpp"
#include "Engine/Render/Ui/UiStrings.hpp"
#include "Engine/Render/Ui/UiConstants.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include <imgui.h>

namespace SettingsWidgetDetails
{
    constexpr float widgetWidth = 350.0f;

    static bool showDemoWindow = false;

    // Shadow values for checkboxes
    static bool vSync = false;
    static bool occlusionCulling = false;

    template <typename T>
    static void Combo(const char* label, const std::span<const T> options, std::function<T()> get, std::function<void(T)> set)
    {
        using namespace UiStrings;
        
        if (options.size() == 1)
        {
            return;
        }
        
        const T currentValue = get();
        
        if (ImGui::BeginCombo(label, ToString(currentValue).data()))
        {
            for (const T option : options)
            {
                const bool isSelected = option == currentValue;

                if (ImGui::Selectable(ToString(option).data(), isSelected))
                {
                    set(option);
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    }

    static void Checkbox(const char* label, bool* shadowValue, std::function<void(bool)> set)
    {
        if (ImGui::Checkbox(label, shadowValue))
        {
            set(*shadowValue);
        }
    }
}

SettingsWidget::SettingsWidget(EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , eventSystem{ &aEventSystem }
    , renderOptions{ &RenderOptions::Get() }
{
    using namespace SettingsWidgetDetails;
    
    std::ranges::copy_if(OptionValues::graphicsPipelineTypes, std::back_inserter(supportedGraphicsPipelineTypes),
        [&](const GraphicsPipelineType type) { return renderOptions->IsGraphicsPipelineTypeSupported(type); });
    std::ranges::copy_if(OptionValues::msaaSampleCounts, std::back_inserter(supportedMsaaSampleCounts),
        [&](const VkSampleCountFlagBits sampleCount) { return renderOptions->IsMsaaSampleCountSupported(sampleCount); });
    
    SettingsWidgetDetails::vSync = renderOptions->GetVSync();
    SettingsWidgetDetails::occlusionCulling = renderOptions->GetOcclusionCulling();
    
    eventSystem->Subscribe<RenderOptions::VSyncChanged>(this, &SettingsWidget::OnVSyncChanged);
    eventSystem->Subscribe<RenderOptions::OcclusionCullingChanged>(this, &SettingsWidget::OnOcclusionCullingChanged);
}

SettingsWidget::~SettingsWidget()
{
    eventSystem->UnsubscribeAll(this);
}

void SettingsWidget::Process(const Frame& frame, float deltaSeconds)
{}

void SettingsWidget::Build()
{
    using namespace SettingsWidgetDetails;

    const ImGuiIO& io = ImGui::GetIO();
    const auto position = ImVec2(io.DisplaySize.x - UiConstants::padding, UiConstants::padding);
    const float maxHeight = io.DisplaySize.y - 2 * UiConstants::padding;

    ImGui::SetNextWindowPos(position, ImGuiCond_None, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(widgetWidth, 0), ImVec2(widgetWidth, maxHeight));
    
    ImGui::Begin("Settings", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
    
    if (ImGui::CollapsingHeader("Render options", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Checkbox("VSync", &vSync, [&](const bool aVSync) { renderOptions->SetVSync(aVSync); });
        ImGui::SameLine();
        Checkbox("Occlusion culling", &occlusionCulling, [&](const bool aOcclusionCulling) { renderOptions->SetOcclusionCulling(aOcclusionCulling); });
        
        Combo<RendererType>("Renderer", OptionValues::rendererTypes,
            [&]() { return renderOptions->GetRendererType(); },
            [&](auto type) { renderOptions->SetRendererType(type); });
    
        if (renderOptions->GetRendererType() == RendererType::eForward)
        {
            Combo<GraphicsPipelineType>("Pipeline", supportedGraphicsPipelineTypes,
                [&]() { return renderOptions->GetGraphicsPipelineType(); },
                [&](auto type) { renderOptions->SetGraphicsPipelineType(type); });
            
            int drawCount = renderOptions->GetCurrentDrawCount();
            if (ImGui::SliderInt("Draw count", &drawCount, 1, renderOptions->GetMaxDrawCount()))
            {
                renderOptions->SetCurrentDrawCount(drawCount);
            }
            
            Combo<VkSampleCountFlagBits>("MSAA sample count", supportedMsaaSampleCounts,
                [&]() { return renderOptions->GetMsaaSampleCount(); },
                [&](auto count) { renderOptions->SetMsaaSampleCount(count); });
            
            int depthMipToVisualize = renderOptions->GetDepthMipToVisualize();
            if (ImGui::SliderInt("Depth mip", &depthMipToVisualize, 0, renderOptions->GetMaxDepthMipToVisualize()))
            {
                renderOptions->SetDepthMipToVisualize(depthMipToVisualize);
            }
        }
    }
    
    if (ImGui::CollapsingHeader("Misc."))
    {
        ImGui::Checkbox("Show demo window", &showDemoWindow);
    }
    
    ImGui::End();

    if (showDemoWindow)
    {
        ImGui::ShowDemoWindow();
    }
}

void SettingsWidget::OnVSyncChanged()
{
    SettingsWidgetDetails::vSync = renderOptions->GetVSync();
}

void SettingsWidget::OnOcclusionCullingChanged()
{
    SettingsWidgetDetails::occlusionCulling = renderOptions->GetOcclusionCulling();
}

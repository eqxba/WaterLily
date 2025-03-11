#include "Engine/Render/Ui/SettingsWidget.hpp"

#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Ui/UiConstants.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include <imgui.h>

namespace SettingsWidgetDetails
{
    constexpr float widgetWidth = 350.0f;

    static bool showDemoWindow = false;
}

SettingsWidget::SettingsWidget(const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
{}

void SettingsWidget::Process(float deltaSeconds)
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
        ImGui::Combo("Renderer", &RenderOptions::renderer, "Scene\0Compute\0\0");

        if (RenderOptions::renderer == 0)
        {
            // TODO: There's a better way to do this 100%
            ImGui::Combo("Pipeline", reinterpret_cast<int*>(&RenderOptions::pipeline), "Mesh\0Vertex\0\0");
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

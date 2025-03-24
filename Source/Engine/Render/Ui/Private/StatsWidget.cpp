#include "Engine/Render/Ui/StatsWidget.hpp"

#include "Engine/Scene/Scene.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include <imgui.h>

StatsWidget::StatsWidget(const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
{}

void StatsWidget::Process(const Frame& frame, float deltaSeconds)
{
    std::ranges::rotate(frameTimes, frameTimes.begin() + 1);
    frameTimes.back() = deltaSeconds;

    triangleCount = frame.stats.triangleCount;
}

void StatsWidget::Build()
{
    ImGui::SetNextWindowPos({});

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.7f));

    ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_AlwaysAutoResize);

    const VkPhysicalDeviceProperties& deviceProperties = vulkanContext->GetDevice().GetProperties().physicalProperties;

    ImGui::TextUnformatted(deviceProperties.deviceName);
    
    ImGui::Text("Vulkan API %i.%i.%i", VK_API_VERSION_MAJOR(deviceProperties.apiVersion),
        VK_API_VERSION_MINOR(deviceProperties.apiVersion), VK_API_VERSION_PATCH(deviceProperties.apiVersion));
    
    const float avgFrameTimeSeconds = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) / frameTimes.size();

    ImGui::Text("Frame time: %.2f ms.", avgFrameTimeSeconds * 1000.0f);

    ImGui::Text("Triangles (total): %.2fM", Scene::GetTotalTriangles() / 1'000'000.0f);
    ImGui::Text("Triangles: %.2fM", triangleCount / 1'000'000.0f);
    
    ImGui::End();

    ImGui::PopStyleColor();
}

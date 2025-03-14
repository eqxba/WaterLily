#include "Engine/Render/Ui/StatsWidget.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include <imgui.h>

StatsWidget::StatsWidget(const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
{}

void StatsWidget::Process(float deltaSeconds)
{
    std::ranges::rotate(frameTimes, frameTimes.begin() + 1);
    frameTimes.back() = deltaSeconds;
}

void StatsWidget::Build()
{
    ImGui::SetNextWindowPos({});

    ImGui::Begin("Stats", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoBackground);

    const VkPhysicalDeviceProperties& deviceProperties = vulkanContext->GetDevice().GetProperties().physicalProperties;

    ImGui::TextUnformatted(deviceProperties.deviceName);
    
    ImGui::Text("Vulkan API %i.%i.%i", VK_API_VERSION_MAJOR(deviceProperties.apiVersion),
        VK_API_VERSION_MINOR(deviceProperties.apiVersion), VK_API_VERSION_PATCH(deviceProperties.apiVersion));
    
    const float avgFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) / frameTimes.size();
    const float fps = (avgFrameTime > 0.0f) ? (1.0f / avgFrameTime) : 0.0f;
    
    ImGui::Text("Fps: %.2f", fps);
    
    ImGui::End();
}

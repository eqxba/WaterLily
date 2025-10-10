#include "Engine/Render/Ui/StatsWidget.hpp"

#include "Engine/Scene/Scene.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Vulkan/StatsUtils.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include <imgui.h>

StatsWidget::StatsWidget(const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
{}

void StatsWidget::Process(const Frame& frame, const float deltaSeconds)
{
    using namespace StatsUtils;
    
    cpuFrameTimesSeconds.Push(deltaSeconds);
    gpuFrameTimesMs.Push(GetTimingMs(vulkanContext->GetDevice(), frame.renderStats, GpuTiming::eFrame));
    firstPassTimesMs.Push(GetTimingMs(vulkanContext->GetDevice(), frame.renderStats, GpuTiming::eFirstPass));
    secondPassTimesMs.Push(GetTimingMs(vulkanContext->GetDevice(), frame.renderStats, GpuTiming::eSecondPass));
    depthPyramidTimesMs.Push(GetTimingMs(vulkanContext->GetDevice(), frame.renderStats, GpuTiming::eDepthPyramid));

    triangleCount = frame.renderStats.triangleCount;
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

    ImGui::Text("CPU Frame time: %.2f ms.", cpuFrameTimesSeconds.GetAverage() * 1000.0f);
    ImGui::Text("GPU Frame time: %.2f ms.", gpuFrameTimesMs.GetAverage());
    
    if (RenderOptions::Get().GetShowExtraGpuTimings())
    {
        ImGui::Text("First pass: %.2f ms.", firstPassTimesMs.GetAverage());
        ImGui::Text("Second pass: %.2f ms.", secondPassTimesMs.GetAverage());
        ImGui::Text("Depth pyramid: %.2f ms.", depthPyramidTimesMs.GetAverage());
    }

    ImGui::Text("Triangles (total): %.2fM", Scene::GetTotalTriangles() / 1'000'000.0f);
    ImGui::Text("Triangles: %.2fM", triangleCount / 1'000'000.0f);
    
    ImGui::End();

    ImGui::PopStyleColor();
}

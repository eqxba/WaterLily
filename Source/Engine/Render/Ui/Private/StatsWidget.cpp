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
    firstCullingPassTimesMs.Push(GetTimingMs(vulkanContext->GetDevice(), frame.renderStats, GpuTiming::eFirstCullingPass));
    secondCullingPassTimesMs.Push(GetTimingMs(vulkanContext->GetDevice(), frame.renderStats, GpuTiming::eSecondCullingPass));
    firstRenderPassTimesMs.Push(GetTimingMs(vulkanContext->GetDevice(), frame.renderStats, GpuTiming::eFirstRenderPass));
    secondRenderPassTimesMs.Push(GetTimingMs(vulkanContext->GetDevice(), frame.renderStats, GpuTiming::eSecondRenderPass));
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
    
    if (const RenderOptions& renderOptions = RenderOptions::Get(); renderOptions.GetShowExtraGpuTimings())
    {
        const bool occlusionCulling = renderOptions.GetOcclusionCulling();
        
        ImGui::Text("First culling pass: %.2f ms.", firstCullingPassTimesMs.GetAverage());
        
        if (occlusionCulling)
        {
            ImGui::Text("Second culling pass: %.2f ms.", secondCullingPassTimesMs.GetAverage());
        }
        
        ImGui::Text("First render pass: %.2f ms.", firstRenderPassTimesMs.GetAverage());
        
        if (occlusionCulling)
        {
            ImGui::Text("Second render pass: %.2f ms.", secondRenderPassTimesMs.GetAverage());
            ImGui::Text("Depth pyramid: %.2f ms.", depthPyramidTimesMs.GetAverage());
        }
    }

    ImGui::Text("Triangles (total): %.2fM", Scene::GetTotalTriangles() / 1'000'000.0f);
    ImGui::Text("Triangles: %.2fM", triangleCount / 1'000'000.0f);
    
    ImGui::End();

    ImGui::PopStyleColor();
}

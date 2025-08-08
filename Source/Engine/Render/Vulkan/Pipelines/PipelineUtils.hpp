#pragma once

#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"

namespace PipelineUtils
{
    void PushConstants(const VkCommandBuffer commandBuffer, const Pipeline& pipeline, const std::string& name, const std::span<const std::byte> data);

    template<typename T>
    void PushConstants(const VkCommandBuffer commandBuffer, const Pipeline& pipeline, const std::string& name, const T& value);
}

template<typename T>
void PipelineUtils::PushConstants(const VkCommandBuffer commandBuffer, const Pipeline& pipeline, const std::string& name, const T& value)
{
    static_assert(std::is_trivially_copyable_v<T>, "PushConstants only supports trivially copyable types");

    const std::byte* bytes = reinterpret_cast<const std::byte*>(&value);
    const std::span<const std::byte> span(bytes, sizeof(T));
    
    PushConstants(commandBuffer, pipeline, name, span);
}

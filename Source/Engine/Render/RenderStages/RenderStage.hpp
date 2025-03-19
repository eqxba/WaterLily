#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/Render/Vulkan/Frame.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

class RenderStage
{
public:
    RenderStage(const VulkanContext& vulkanContext, RenderContext& renderContext);
    virtual ~RenderStage();
    
    RenderStage(const RenderStage&) = delete;
    RenderStage& operator=(const RenderStage&) = delete;

    RenderStage(RenderStage&&) = delete;
    RenderStage& operator=(RenderStage&&) = delete;
    
    virtual void Prepare(const Scene& scene);
    
    virtual void Execute(const Frame& frame);
    
    virtual void RecreateFramebuffers();
    virtual void TryReloadShaders();
    
protected:
    const VulkanContext* vulkanContext = nullptr;
    RenderContext* renderContext = nullptr;
};

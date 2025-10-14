#pragma once

#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/RenderContext.hpp"

class VulkanContext;
class EventSystem;
class PrimitiveCullStage;
class RenderStage;
class Scene;

namespace ES
{
    struct SceneOpened;
}

class ForwardRenderer : public Renderer
{
public:
    ForwardRenderer(EventSystem& eventSystem, const VulkanContext& vulkanContext);
    ~ForwardRenderer() override;

    ForwardRenderer(const ForwardRenderer&) = delete;
    ForwardRenderer& operator=(const ForwardRenderer&) = delete;

    ForwardRenderer(ForwardRenderer&&) = delete;
    ForwardRenderer& operator=(ForwardRenderer&&) = delete;

    void Process(const Frame& frame, float deltaSeconds) override;

    void Render(const Frame& frame) override;

private:
    void RenderWithOcclusionCulling(const Frame& frame);
    
    void InitRuntimeDefineGetters();
    
    void CreateRenderPasses();
    void DestroyRenderPasses();
    
    void CreateRenderTargets();
    void DestroyRenderTargets();
    
    void CreateFramebuffers();
    void DestroyFramebuffers();
    
    void Reinitialize();

    void OnBeforeSwapchainRecreated();
    void OnSwapchainRecreated();
    void OnTryReloadShaders();
    void OnSceneOpen(const ES::SceneOpened& event);
    void OnSceneClose();

    const VulkanContext* vulkanContext = nullptr;

    EventSystem* eventSystem = nullptr;
    
    RenderContext renderContext;

    std::unique_ptr<PrimitiveCullStage> primitiveCullStage;
    std::unique_ptr<RenderStage> forwardStage;
    std::unique_ptr<RenderStage> debugStage;
    
    std::vector<RenderStage*> renderStages;

    Scene* scene = nullptr;
};

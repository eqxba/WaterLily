#pragma once

#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/RenderContext.hpp"

class VulkanContext;
class EventSystem;
class RenderStage;
class Scene;

namespace ES
{
    struct SceneOpened;
}

class SceneRenderer : public Renderer
{
public:
    SceneRenderer(EventSystem& eventSystem, const VulkanContext& vulkanContext);
    ~SceneRenderer() override;

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    SceneRenderer(SceneRenderer&&) = delete;
    SceneRenderer& operator=(SceneRenderer&&) = delete;

    void Process(const Frame& frame, float deltaSeconds) override;

    void Render(const Frame& frame) override;

private:
    void CreateRenderTargets();
    void DestroyRenderTargets();
    
    void PrepareGlobalDefines();

    void OnBeforeSwapchainRecreated();
    void OnSwapchainRecreated();
    void OnTryReloadShaders();
    void OnSceneOpen(const ES::SceneOpened& event);
    void OnSceneClose();

    const VulkanContext* vulkanContext = nullptr;

    EventSystem* eventSystem = nullptr;
    
    RenderContext renderContext;

    std::unique_ptr<RenderStage> primitiveCullStage;
    std::unique_ptr<RenderStage> forwardStage;

    Scene* scene = nullptr;
};

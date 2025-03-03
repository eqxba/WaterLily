#pragma once

#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/Instance.hpp"
#include "Engine/Render/Vulkan/Surface.hpp"
#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Swapchain.hpp"
#include "Engine/Render/Vulkan/Managers/MemoryManager.hpp"
#include "Engine/Render/Vulkan/Managers/ShaderManager.hpp"
#include "Engine/Render/Vulkan/Managers/DescriptorSetManager.hpp"

namespace ES
{
    struct WindowResized;
    struct BeforeWindowRecreated;
    struct WindowRecreated;
}

class Window;

class VulkanContext
{
public:
    VulkanContext(const Window& window, EventSystem& eventSystem);
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;

    const Instance& GetInstance() const
    {
        return *instance;
    }

    const Surface& GetSurface() const
    {
        return *surface;
    }

    const Device& GetDevice() const
    {
        return *device;
    }

    Swapchain& GetSwapchain() const
    {
        return *swapchain;
    }

    MemoryManager& GetMemoryManager() const
    {
        return *memoryManager;
    }

    const ShaderManager& GetShaderManager() const
    {
        return *shaderManager;
    }

    DescriptorSetManager& GetDescriptorSetsManager() const
    {
        return *descriptorSetsManager;
    }

private:
    void OnResize(const ES::WindowResized& event);
    void OnBeforeWindowRecreated(const ES::BeforeWindowRecreated& event);
    void OnWindowRecreated(const ES::WindowRecreated& event);

    EventSystem& eventSystem;

    std::unique_ptr<Instance> instance;
    std::unique_ptr<Surface> surface;
    std::unique_ptr<Device> device;
    std::unique_ptr<Swapchain> swapchain;

    std::unique_ptr<MemoryManager> memoryManager;
    std::unique_ptr<ShaderManager> shaderManager;
    std::unique_ptr<DescriptorSetManager> descriptorSetsManager;
};

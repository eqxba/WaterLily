#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Window.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Render/RenderOptions.hpp"

#define VOLK_IMPLEMENTATION
#include <volk.h>

namespace VulkanContextDetails
{
    static bool volkInitialized = false;

    static VkExtent2D ToVkExtent2D(const Extent2D windowExtent)
    {
        return { static_cast<uint32_t>(windowExtent.width), static_cast<uint32_t>(windowExtent.height) };
    }
}

VulkanContext::VulkanContext(const Window& window, EventSystem& aEventSystem)
    : eventSystem{ aEventSystem }
{
    using namespace VulkanContextDetails;
    
    if (!volkInitialized)
    {
        const VkResult volkInitializeResult = volkInitialize();
        Assert(volkInitializeResult == VK_SUCCESS);
        volkInitialized = true;
    }

    instance = std::make_unique<Instance>();
    surface = std::make_unique<Surface>(window, *this);
    device = std::make_unique<Device>(*this);
    swapchain = std::make_unique<Swapchain>(ToVkExtent2D(window.GetExtentInPixels()), *this);

    memoryManager = std::make_unique<MemoryManager>(*this);
    shaderManager = std::make_unique<ShaderManager>(*this);
    descriptorSetsManager = std::make_unique<DescriptorSetManager>(*this);
    
    RenderOptions::Initialize(*this, eventSystem);

    eventSystem.Subscribe<ES::WindowResized>(this, &VulkanContext::OnResize);
    eventSystem.Subscribe<ES::BeforeWindowRecreated>(this, &VulkanContext::OnBeforeWindowRecreated);
    eventSystem.Subscribe<ES::WindowRecreated>(this, &VulkanContext::OnWindowRecreated);
}

VulkanContext::~VulkanContext()
{
    eventSystem.UnsubscribeAll(this);
}

void VulkanContext::OnResize(const ES::WindowResized& event)
{
    if (event.newExtent.width == 0 || event.newExtent.height == 0)
    {
        return;
    }
    
    device->WaitIdle();
    
    eventSystem.Fire<ES::BeforeSwapchainRecreated>();
    
    swapchain->Recreate(VulkanContextDetails::ToVkExtent2D(event.newExtent));
    
    eventSystem.Fire<ES::SwapchainRecreated>();
}

void VulkanContext::OnBeforeWindowRecreated(const ES::BeforeWindowRecreated& event)
{
    device->WaitIdle();
    
    eventSystem.Fire<ES::BeforeSwapchainRecreated>();
    
    swapchain.reset();
    surface.reset();
}

void VulkanContext::OnWindowRecreated(const ES::WindowRecreated& event)
{
    surface = std::make_unique<Surface>(*event.window, *this);
    swapchain = std::make_unique<Swapchain>(VulkanContextDetails::ToVkExtent2D(event.window->GetExtentInPixels()), *this);
    
    eventSystem.Fire<ES::SwapchainRecreated>();
}

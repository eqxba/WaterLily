#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Window.hpp"
#include "Engine/EventSystem.hpp"

#define VOLK_IMPLEMENTATION
#include <volk.h>

static bool volkInitialized = false;

VulkanContext::VulkanContext(const Window& window, EventSystem& aEventSystem)
    : eventSystem{ aEventSystem }
{
    if (!volkInitialized) 
    {
        const VkResult volkInitializeResult = volkInitialize();
        Assert(volkInitializeResult == VK_SUCCESS);
        volkInitialized = true;
    }

    instance = std::make_unique<Instance>();
    surface = std::make_unique<Surface>(window, *this);
    device = std::make_unique<Device>(*this);
    swapchain = std::make_unique<Swapchain>(window.GetExtentInPixels(), *this);

    memoryManager = std::make_unique<MemoryManager>(*this);
    shaderManager = std::make_unique<ShaderManager>(*this);
    descriptorSetsManager = std::make_unique<DescriptorSetManager>(*this);

    eventSystem.Subscribe<ES::BeforeWindowRecreated>(this, &VulkanContext::OnBeforeWindowRecreated);
    eventSystem.Subscribe<ES::WindowRecreated>(this, &VulkanContext::OnWindowRecreated);
}

VulkanContext::~VulkanContext()
{
    eventSystem.Unsubscribe<ES::BeforeWindowRecreated>(this);
    eventSystem.Unsubscribe<ES::WindowRecreated>(this);
}

void VulkanContext::OnBeforeWindowRecreated(const ES::BeforeWindowRecreated& event)
{
    swapchain.reset();
    surface.reset();
}

void VulkanContext::OnWindowRecreated(const ES::WindowRecreated& event)
{
    surface = std::make_unique<Surface>(*event.window, *this);
    swapchain = std::make_unique<Swapchain>(event.window->GetExtentInPixels(), *this);
}
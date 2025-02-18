#pragma once

#include <volk.h>

class VulkanContext;

class RenderPass
{
public:
    RenderPass(const VulkanContext& aVulkanContext);
	~RenderPass();

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    RenderPass(RenderPass&&) = delete;
    RenderPass& operator=(RenderPass&&) = delete;

    VkRenderPass GetVkRenderPass() const
    {
        return renderPass;
    }

private:
    const VulkanContext& vulkanContext;

    VkRenderPass renderPass;
};
#include "Engine/Systems/RenderSystem.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace RenderSystemDetails
{
	static std::vector<VkFramebuffer> CreateFramebuffers(VkRenderPass renderPass)
	{
		std::vector<VkFramebuffer> framebuffers;
		framebuffers.reserve(VulkanContext::swapchain->imageViews.size());

		for (const auto& imageView : VulkanContext::swapchain->imageViews)
		{
            VkImageView attachments[] = { imageView };
			
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = VulkanContext::swapchain->extent.width;
            framebufferInfo.height = VulkanContext::swapchain->extent.height;
            framebufferInfo.layers = 1;

            VkFramebuffer framebuffer;
            const VkResult result = 
				vkCreateFramebuffer(VulkanContext::device->device, &framebufferInfo, nullptr, &framebuffer);
            Assert(result == VK_SUCCESS);

            framebuffers.push_back(framebuffer);
		}

        return framebuffers;
	}
}

RenderSystem::RenderSystem()
	: graphicsPipeline(renderPass)
{
	using namespace RenderSystemDetails;
	
	swapchainFramebuffers = CreateFramebuffers(renderPass.renderPass);
}

RenderSystem::~RenderSystem()
{
	for (const auto& framebuffer : swapchainFramebuffers) 
	{
		vkDestroyFramebuffer(VulkanContext::device->device, framebuffer, nullptr);
	}
}

void RenderSystem::Process(float deltaSeconds)
{}

void RenderSystem::Render()
{
	
}
#include "Engine/Systems/RenderSystem.hpp"

#include "Engine/Engine.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

namespace RenderSystemDetails
{
	static std::vector<VkFramebuffer> CreateFramebuffers(VkRenderPass renderPass)
	{
		std::vector<VkFramebuffer> framebuffers;
		framebuffers.reserve(VulkanContext::swapchain->imageViews.size());

		for (const auto imageView : VulkanContext::swapchain->imageViews)
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

	static VkCommandPool CreateCommandPool()
	{		
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = VulkanContext::device->queues.familyIndices.graphicsFamily;
		poolInfo.flags = 0;

		VkCommandPool commandPool;
		const VkResult result = vkCreateCommandPool(VulkanContext::device->device, &poolInfo, nullptr, &commandPool);
		Assert(result == VK_SUCCESS);

		return commandPool;
	}

	static std::vector<VkCommandBuffer> CreateCommandBuffers(VkCommandPool commandPool, const RenderPass& renderPass,
		const std::vector<VkFramebuffer>& framebuffers, const GraphicsPipeline& graphicsPipeline)
	{
		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.resize(framebuffers.size());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		VkResult result = vkAllocateCommandBuffers(VulkanContext::device->device, &allocInfo, commandBuffers.data());
		Assert(result == VK_SUCCESS);

		for (size_t i = 0; i < commandBuffers.size(); ++i)
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;

			result = vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
			Assert(result == VK_SUCCESS);

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass.renderPass;
			renderPassInfo.framebuffer = framebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = VulkanContext::swapchain->extent;

			VkClearValue clearColor = { 0.73f, 0.95f, 1.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipeline);
			vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
			vkCmdEndRenderPass(commandBuffers[i]);

			result = vkEndCommandBuffer(commandBuffers[i]);
			Assert(result == VK_SUCCESS);
		}
		
		return commandBuffers;
	}

	// TODO: On first reuse move to helpers file or smth (not only this function)
	static std::vector<VkSemaphore> CreateSemaphores(const size_t count)
	{
		std::vector<VkSemaphore> semaphores(count);

		for (auto& semaphore : semaphores)
		{
			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			const VkResult result = 
				vkCreateSemaphore(VulkanContext::device->device, &semaphoreInfo, nullptr, &semaphore);
			Assert(result == VK_SUCCESS);
		}

		return semaphores;
	}

	static void DestroySemaphores(const std::vector<VkSemaphore>& semaphores)
	{
		for (const auto semaphore : semaphores)
		{
			vkDestroySemaphore(VulkanContext::device->device, semaphore, nullptr);
		}
	}

	static std::vector<VkFence> CreateFences(const size_t count)
	{
		std::vector<VkFence> fences(count);

		for (auto& fence : fences)
		{
			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			const VkResult result = vkCreateFence(VulkanContext::device->device, &fenceInfo, nullptr, &fence);
			Assert(result == VK_SUCCESS);
		}

		return fences;
	}

	static void DestroyFramebuffers(std::vector<VkFramebuffer>& framebuffers)
	{
		for (const auto framebuffer : framebuffers)
		{
			vkDestroyFramebuffer(VulkanContext::device->device, framebuffer, nullptr);
		}
	}
}

RenderSystem::RenderSystem()
	: renderPass(std::make_unique<RenderPass>())
	, graphicsPipeline(std::make_unique<GraphicsPipeline>(*renderPass))
	, imagesInFlight(VulkanContext::swapchain->images.size(), VK_NULL_HANDLE)
{
	using namespace RenderSystemDetails;
	
	framebuffers = CreateFramebuffers(renderPass->renderPass);
	commandPool = CreateCommandPool();
	commandBuffers = CreateCommandBuffers(commandPool, *renderPass, framebuffers, *graphicsPipeline);
	imageAvailableSemaphores = CreateSemaphores(VulkanConfig::maxFramesInFlight);
	renderFinishedSemaphores = CreateSemaphores(VulkanConfig::maxFramesInFlight);
	inFlightFences = CreateFences(VulkanConfig::maxFramesInFlight);

	Engine::GetEventSystem()->Subscribe<ES::WindowResized>(this, &RenderSystem::OnResize);
}

RenderSystem::~RenderSystem()
{
	Engine::GetEventSystem()->Unsubscribe<ES::WindowResized>(this);

	for (const auto fence : inFlightFences)
	{
		vkDestroyFence(VulkanContext::device->device, fence, nullptr);
	}
	
	RenderSystemDetails::DestroySemaphores(renderFinishedSemaphores);
	RenderSystemDetails::DestroySemaphores(imageAvailableSemaphores);
	
	vkDestroyCommandPool(VulkanContext::device->device, commandPool, nullptr);
	
	RenderSystemDetails::DestroyFramebuffers(framebuffers);
}

void RenderSystem::Process(float deltaSeconds)
{}

void RenderSystem::Render()
{
	vkWaitForFences(VulkanContext::device->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	
	uint32_t imageIndex;
	const VkResult acquireResult = vkAcquireNextImageKHR(VulkanContext::device->device, 
		VulkanContext::swapchain->swapchain, std::numeric_limits<uint64_t>::max(), 
		imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
	Assert(acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR);
	
	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) 
	{
		vkWaitForFences(VulkanContext::device->device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	// Mark the image as now being in use by this frame
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(VulkanContext::device->device, 1, &inFlightFences[currentFrame]);
	
	const VkResult result = 
		vkQueueSubmit(VulkanContext::device->queues.graphics, 1, &submitInfo, inFlightFences[currentFrame]);
	Assert(result == VK_SUCCESS);

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { VulkanContext::swapchain->swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	const VkResult presentResult = vkQueuePresentKHR(VulkanContext::device->queues.present, &presentInfo);
	Assert(presentResult == VK_SUCCESS);

	currentFrame = (currentFrame + 1) % VulkanConfig::maxFramesInFlight;
}

void RenderSystem::OnResize()
{
	using namespace RenderSystemDetails;

	DestroyFramebuffers(framebuffers);
	vkFreeCommandBuffers(VulkanContext::device->device, commandPool, static_cast<uint32_t>(commandBuffers.size()),
		commandBuffers.data());
	
	graphicsPipeline = std::make_unique<GraphicsPipeline>(*renderPass);
	framebuffers = CreateFramebuffers(renderPass->renderPass);
	commandBuffers = CreateCommandBuffers(commandPool, *renderPass, framebuffers, *graphicsPipeline);
}
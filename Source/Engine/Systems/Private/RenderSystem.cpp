#include "Engine/Systems/RenderSystem.hpp"

#include "Engine/Engine.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Scene/Scene.hpp"

namespace RenderSystemDetails
{
	static std::vector<VkFramebuffer> CreateFramebuffers(const VulkanContext& vulkanContext, VkRenderPass renderPass)
	{
		const Swapchain& swapchain = vulkanContext.GetSwapchain();

		std::vector<VkFramebuffer> framebuffers;
		framebuffers.reserve(swapchain.GetImageViews().size());

		for (const auto imageView : swapchain.GetImageViews())
		{
            VkImageView attachments[] = { imageView };
			
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapchain.GetExtent().width;
            framebufferInfo.height = swapchain.GetExtent().height;
            framebufferInfo.layers = 1;

            VkFramebuffer framebuffer;
            const VkResult result = 
				vkCreateFramebuffer(vulkanContext.GetDevice().GetVkDevice(), &framebufferInfo, nullptr, &framebuffer);
            Assert(result == VK_SUCCESS);

            framebuffers.push_back(framebuffer);
		}

        return framebuffers;
	}

	static std::vector<VkCommandBuffer> CreateCommandBuffers(const VulkanContext& vulkanContext, 
		VkCommandPool commandPool, const RenderPass& renderPass, const std::vector<VkFramebuffer>& framebuffers, 
		const GraphicsPipeline& graphicsPipeline, Scene& scene)
	{
		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.resize(framebuffers.size());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		VkResult result = vkAllocateCommandBuffers(vulkanContext.GetDevice().GetVkDevice(), &allocInfo, commandBuffers.data());
		Assert(result == VK_SUCCESS);

		for (size_t i = 0; i < commandBuffers.size(); ++i)
		{
			// FYI: vkBeginCommandBuffer if buffer was created with VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag
			// implicitly resets the buffer to the initial state
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;

			result = vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
			Assert(result == VK_SUCCESS);

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass.GetVkRenderPass();
			renderPassInfo.framebuffer = framebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = vulkanContext.GetSwapchain().GetExtent();

			VkClearValue clearColor = { 0.73f, 0.95f, 1.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetVkPipeline());

            VkBuffer vertexBuffers[] = { scene.vertexBuffer };
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffers[i], scene.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(scene.indices.size()), 1, 0, 0, 0);
			vkCmdEndRenderPass(commandBuffers[i]);

			result = vkEndCommandBuffer(commandBuffers[i]);
			Assert(result == VK_SUCCESS);
		}
		
		return commandBuffers;
	}

	static void DestroyFramebuffers(std::vector<VkFramebuffer>& framebuffers, VkDevice device)
	{
		for (const auto framebuffer : framebuffers)
		{
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}
	}
}

RenderSystem::RenderSystem(Scene& aScene, EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
	: vulkanContext{aVulkanContext}
	, eventSystem{aEventSystem}
	, renderPass{std::make_unique<RenderPass>(vulkanContext)}
	, graphicsPipeline{std::make_unique<GraphicsPipeline>(*renderPass, vulkanContext)}
	, imagesInFlight{vulkanContext.GetSwapchain().GetImages().size(), VK_NULL_HANDLE}
    , scene{aScene}
{
	using namespace RenderSystemDetails;

	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();
	const VkCommandPool longLivedPool = vulkanContext.GetDevice().GetCommandPool(CommandBufferType::eLongLived);
	
	framebuffers = CreateFramebuffers(vulkanContext, renderPass->GetVkRenderPass());
	commandBuffers = CreateCommandBuffers(vulkanContext, longLivedPool, *renderPass, framebuffers, *graphicsPipeline, 
		scene);
	imageAvailableSemaphores = VulkanHelpers::CreateSemaphores(device, VulkanConfig::maxFramesInFlight);
	renderFinishedSemaphores = VulkanHelpers::CreateSemaphores(device, VulkanConfig::maxFramesInFlight);
	inFlightFences = VulkanHelpers::CreateFences(device, VK_FENCE_CREATE_SIGNALED_BIT, VulkanConfig::maxFramesInFlight);

	eventSystem.Subscribe<ES::WindowResized>(this, &RenderSystem::OnResize);
}

RenderSystem::~RenderSystem()
{
	eventSystem.Unsubscribe<ES::WindowResized>(this);

	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();

	VulkanHelpers::DestroyFences(device, inFlightFences);
	VulkanHelpers::DestroySemaphores(device, renderFinishedSemaphores);
	VulkanHelpers::DestroySemaphores(device, imageAvailableSemaphores);
	
	RenderSystemDetails::DestroyFramebuffers(framebuffers, device);
}

void RenderSystem::Process(float deltaSeconds)
{}

void RenderSystem::Render()
{
	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();
	const VkSwapchainKHR swapchain = vulkanContext.GetSwapchain().GetVkSwapchainKHR();

	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	
	uint32_t imageIndex;
	const VkResult acquireResult = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), 
		imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
	Assert(acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR);
	
	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) 
	{
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
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

	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	const Queues& queues = vulkanContext.GetDevice().GetQueues();
	
	const VkResult result = vkQueueSubmit(queues.graphics, 1, &submitInfo, inFlightFences[currentFrame]);
	Assert(result == VK_SUCCESS);

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	const VkResult presentResult = vkQueuePresentKHR(queues.present, &presentInfo);
	Assert(presentResult == VK_SUCCESS);

	currentFrame = (currentFrame + 1) % VulkanConfig::maxFramesInFlight;
}

void RenderSystem::OnResize(const ES::WindowResized& event)
{
	using namespace RenderSystemDetails;

	vulkanContext.GetDevice().WaitIdle();

	if (event.newWidth != 0 && event.newHeight != 0)
	{
		vulkanContext.GetSwapchain().Recreate({event.newWidth, event.newHeight});
	}

	const Device& device = vulkanContext.GetDevice();

	DestroyFramebuffers(framebuffers, device.GetVkDevice());
	// TODO: Do we have to recreate 'em here?
	vkFreeCommandBuffers(device.GetVkDevice(), device.GetCommandPool(CommandBufferType::eLongLived),
		static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	
	graphicsPipeline = std::make_unique<GraphicsPipeline>(*renderPass, vulkanContext);
	framebuffers = CreateFramebuffers(vulkanContext, renderPass->GetVkRenderPass());
	commandBuffers = CreateCommandBuffers(vulkanContext, device.GetCommandPool(CommandBufferType::eLongLived),
		*renderPass, framebuffers, *graphicsPipeline, scene);
}
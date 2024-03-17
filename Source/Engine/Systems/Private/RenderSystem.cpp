#include "Engine/Systems/RenderSystem.hpp"

#include "Engine/Engine.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Scene/Scene.hpp"

namespace RenderSystemDetails
{
	static std::vector<VkFramebuffer> CreateFramebuffers(const VulkanContext& vulkanContext, VkRenderPass renderPass)
	{
		const Swapchain& swapchain = vulkanContext.GetSwapchain();

		std::vector<VkFramebuffer> framebuffers;
		framebuffers.reserve(swapchain.GetImageViews().size());

		const auto createFramebuffer = [&](VkImageView imageView) {
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
			const VkResult result = vkCreateFramebuffer(vulkanContext.GetDevice().GetVkDevice(), 
				&framebufferInfo, nullptr, &framebuffer);
			Assert(result == VK_SUCCESS);

			return framebuffer;
		};

		std::ranges::transform(swapchain.GetImageViews(), std::back_inserter(framebuffers), createFramebuffer);

        return framebuffers;
	}

	static VkViewport GetViewport(const VkExtent2D extent)
	{
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(extent.width);
		viewport.height = static_cast<float>(extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		return viewport;
	}

	static VkRect2D GetScissor(const VkExtent2D extent)
	{
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = extent;

		return scissor;
	}

	static std::vector<VkCommandBuffer> CreateCommandBuffers(const VulkanContext& vulkanContext, const size_t count,
		VkCommandPool commandPool)
	{
		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.resize(count);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		VkResult result = vkAllocateCommandBuffers(vulkanContext.GetDevice().GetVkDevice(), &allocInfo, commandBuffers.data());
		Assert(result == VK_SUCCESS);		
		
		return commandBuffers;
	}

	static std::vector<CommandBufferSync> CreateCommandBufferSyncs(const VulkanContext& vulkanContext, const size_t count)
	{
		std::vector<CommandBufferSync> syncs(count);		

		const VkDevice device = vulkanContext.GetDevice().GetVkDevice();

		std::ranges::generate(syncs, [&]() {
			CommandBufferSync sync;
			sync.waitSemaphores = VulkanHelpers::CreateSemaphores(device, 1);
			sync.waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			sync.signalSemaphores = VulkanHelpers::CreateSemaphores(device, 1);
			sync.fence = VulkanHelpers::CreateFence(device, VK_FENCE_CREATE_SIGNALED_BIT);
			return sync;
		});

		return syncs;
	}

	static void EnqueueRenderCommands(const VulkanContext& vulkanContext, VkCommandBuffer commandBuffer, 
		VkFramebuffer framebuffer, const RenderPass& renderPass, const GraphicsPipeline& graphicsPipeline, Scene& scene)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass.GetVkRenderPass();
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = vulkanContext.GetSwapchain().GetExtent();

		VkClearValue clearColor = { 0.73f, 0.95f, 1.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetVkPipeline());

		const VkExtent2D extent = vulkanContext.GetSwapchain().GetExtent();
		VkViewport viewport = GetViewport(extent);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = GetScissor(extent);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = { scene.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, scene.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(scene.indices.size()), 1, 0, 0, 0);
		vkCmdEndRenderPass(commandBuffer);
	}

	static void DestroyFramebuffers(const std::vector<VkFramebuffer>& framebuffers, VkDevice device)
	{
		std::ranges::for_each(framebuffers, [=](VkFramebuffer framebuffer) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		});
	}
}

RenderSystem::RenderSystem(Scene& aScene, EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
	: vulkanContext{aVulkanContext}
	, eventSystem{aEventSystem}
	, renderPass{std::make_unique<RenderPass>(vulkanContext)}
	, graphicsPipeline{std::make_unique<GraphicsPipeline>(*renderPass, vulkanContext)}
    , scene{aScene}
{
	using namespace RenderSystemDetails;

	const VkCommandPool longLivedPool = vulkanContext.GetDevice().GetCommandPool(CommandBufferType::eLongLived);
	
	framebuffers = CreateFramebuffers(vulkanContext, renderPass->GetVkRenderPass());
	commandBuffers = CreateCommandBuffers(vulkanContext, VulkanConfig::maxFramesInFlight, longLivedPool);
	syncs = CreateCommandBufferSyncs(vulkanContext, VulkanConfig::maxFramesInFlight);

	eventSystem.Subscribe<ES::WindowResized>(this, &RenderSystem::OnResize);
}

RenderSystem::~RenderSystem()
{
	eventSystem.Unsubscribe<ES::WindowResized>(this);

	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();	

	std::ranges::for_each(syncs, [device](CommandBufferSync& sync) {
		VulkanHelpers::DestroySemaphores(device, sync.waitSemaphores);
		VulkanHelpers::DestroySemaphores(device, sync.signalSemaphores);
		vkDestroyFence(device, sync.fence, nullptr);
	});
	
	RenderSystemDetails::DestroyFramebuffers(framebuffers, device);
}

void RenderSystem::Process(float deltaSeconds)
{}

void RenderSystem::Render()
{
	using namespace RenderSystemDetails;
	using namespace VulkanHelpers;

	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();
	const VkSwapchainKHR swapchain = vulkanContext.GetSwapchain().GetVkSwapchainKHR();
	CommandBufferSync& sync = syncs[currentFrame];

	vkWaitForFences(device, 1, &sync.fence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &sync.fence);
	
	uint32_t imageIndex;
	const VkResult acquireResult = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), 
		sync.waitSemaphores[0], VK_NULL_HANDLE, &imageIndex);
	Assert(acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR);

	VkCommandBuffer commandBuffer = commandBuffers[currentFrame];
	const Queues& queues = vulkanContext.GetDevice().GetQueues();

	const auto renderCommands = [&](VkCommandBuffer buffer) {
		EnqueueRenderCommands(vulkanContext, buffer, framebuffers[imageIndex], *renderPass, *graphicsPipeline, scene);
	};

	VulkanHelpers::SubmitCommandBuffer(commandBuffer, queues.graphics, renderCommands, sync);

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = sync.signalSemaphores.data();

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

	DestroyFramebuffers(framebuffers, vulkanContext.GetDevice().GetVkDevice());
	framebuffers = CreateFramebuffers(vulkanContext, renderPass->GetVkRenderPass());
}
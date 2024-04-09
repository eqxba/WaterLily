#include "Engine/Systems/RenderSystem.hpp"

#include "Engine/Engine.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Shaders/Common.h"

#include <glm/gtc/matrix_transform.hpp>

namespace RenderSystemDetails
{
	static std::vector<VkFramebuffer> CreateFramebuffers(const Swapchain& swapchain, VkImageView depthImageView,
		VkRenderPass renderPass, VkDevice device)
	{
		std::vector<VkFramebuffer> framebuffers;
		framebuffers.reserve(swapchain.GetImageViews().size());

		const auto createFramebuffer = [&](VkImageView imageView) {
			std::array<VkImageView, 2> attachments = { imageView, depthImageView };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapchain.GetExtent().width;
			framebufferInfo.height = swapchain.GetExtent().height;
			framebufferInfo.layers = 1;

			VkFramebuffer framebuffer;
			const VkResult result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer);
			Assert(result == VK_SUCCESS);

			return framebuffer;
		};

		std::ranges::transform(swapchain.GetImageViews(), std::back_inserter(framebuffers), createFramebuffer);

        return framebuffers;
	}

	static std::unique_ptr<Image> CreateDepthAttachment(VkExtent2D extent, const VulkanContext& vulkanContext)
	{
		ImageDescription imageDescription{ 
			.extent = { static_cast<int>(extent.width), static_cast<int>(extent.height) },
			.format = VulkanConfig::depthImageFormat,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

		return std::make_unique<Image>(imageDescription, &vulkanContext);
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

	static std::vector<CommandBufferSync> CreateCommandBufferSyncs(const size_t count, VkDevice device)
	{
		std::vector<CommandBufferSync> syncs(count);

		std::ranges::generate(syncs, [&]() {
			CommandBufferSync sync{
				VulkanHelpers::CreateSemaphores(device, 1),
				{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
				VulkanHelpers::CreateSemaphores(device, 1), 
				VulkanHelpers::CreateFence(device, VK_FENCE_CREATE_SIGNALED_BIT),
				device
			};
			return sync;
		});

		return syncs;
	}

	static void EnqueueRenderCommands(const VulkanContext& vulkanContext, VkCommandBuffer commandBuffer, 
		VkFramebuffer framebuffer, const RenderPass& renderPass, const GraphicsPipeline& graphicsPipeline, Scene& scene,
		const std::span<const VkDescriptorSet> descriptorSets)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass.GetVkRenderPass();
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = vulkanContext.GetSwapchain().GetExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.73f, 0.95f, 1.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetVkPipeline());

		const VkExtent2D extent = vulkanContext.GetSwapchain().GetExtent();
		VkViewport viewport = GetViewport(extent);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = GetScissor(extent);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = { scene.GetVertexBuffer()->GetVkBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, scene.GetIndexBuffer()->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipelineLayout(), 
			0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(scene.GetIndices().size()), 1, 0, 0, 0);
		vkCmdEndRenderPass(commandBuffer);
	}

	static void DestroyFramebuffers(const std::vector<VkFramebuffer>& framebuffers, VkDevice device)
	{
		std::ranges::for_each(framebuffers, [=](VkFramebuffer framebuffer) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		});
	}

	static std::vector<Buffer> CreateUniformBuffers(const VulkanContext& vulkanContext, const size_t count)
	{
		const auto createBuffer = [&]() {
			BufferDescription bufferDescription{ static_cast<VkDeviceSize>(sizeof(gpu::UniformBufferObject)),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

			auto result = Buffer(bufferDescription, true, &vulkanContext);
			result.GetStagingBuffer()->MapMemory(true);

			return result;
		};

		std::vector<Buffer> result(count);
		std::ranges::generate(result, createBuffer);

		return result;
	}

	static VkDescriptorPool CreateDescriptorPool(const uint32_t descriptorCount, const uint32_t maxSets, VkDevice device)
	{
		VkDescriptorPool descriptorPool;

		std::array<VkDescriptorPoolSize, 2> poolSizes{};

		VkDescriptorPoolSize& uniformBufferPoolSize = poolSizes[0];
		uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferPoolSize.descriptorCount = descriptorCount;

		VkDescriptorPoolSize& samplersPoolSize = poolSizes[1];
		samplersPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplersPoolSize.descriptorCount = descriptorCount;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = maxSets;

		const VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
		Assert(result == VK_SUCCESS);

		return descriptorPool;
	}

	static std::vector<VkDescriptorSet> CreateDescriptorSets(VkDescriptorPool descriptorPool,
		VkDescriptorSetLayout layout, size_t count, VkDevice device)
	{
		std::vector<VkDescriptorSet> descriptorSets(count);

		std::vector<VkDescriptorSetLayout> layouts(count, layout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(count);
		allocInfo.pSetLayouts = layouts.data();

		const VkResult result = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
		Assert(result == VK_SUCCESS);

		return descriptorSets;
	}

	static void PopulateDescriptorSets(const std::vector<VkDescriptorSet>& sets, const std::vector<Buffer>& buffers,
		VkImageView imageView, VkSampler sampler, VkDevice device)
	{
		Assert(sets.size() == buffers.size());

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkWriteDescriptorSet> descriptorWrites;

		bufferInfos.reserve(sets.size());
		descriptorWrites.reserve(sets.size() * 2);

		std::ranges::transform(buffers, std::back_inserter(bufferInfos), [](const Buffer& buffer) {
			return VkDescriptorBufferInfo{ buffer.GetVkBuffer(), 0, sizeof(gpu::UniformBufferObject) };
		});

		const auto createBufferWrite = [](const VkDescriptorSet& descriptorSet, const VkDescriptorBufferInfo& bufferInfo) {
			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSet;
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;

			return descriptorWrite;
		};

		const auto createSamplerWrite = [&imageInfo](const VkDescriptorSet& descriptorSet) {
			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSet;
			descriptorWrite.dstBinding = 1;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pImageInfo = &imageInfo;

			return descriptorWrite;
		};

		// TODO: 1 cycle instead of 2?
		std::ranges::transform(sets, bufferInfos, std::back_inserter(descriptorWrites), createBufferWrite);
		std::ranges::transform(sets, std::back_inserter(descriptorWrites), createSamplerWrite);

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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
	using namespace VulkanHelpers;
	using namespace VulkanConfig;

	const VkCommandPool longLivedPool = vulkanContext.GetDevice().GetCommandPool(CommandBufferType::eLongLived);
	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();

	const Swapchain& swapchain = vulkanContext.GetSwapchain();

	depthAttachment = CreateDepthAttachment(swapchain.GetExtent(), vulkanContext);
	depthImageView = depthAttachment->CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
	framebuffers = CreateFramebuffers(swapchain, depthImageView, renderPass->GetVkRenderPass(), device);
	commandBuffers = CreateCommandBuffers(device, maxFramesInFlight, longLivedPool);
	syncs = CreateCommandBufferSyncs(maxFramesInFlight, device);

	VkDescriptorSetLayout layout = graphicsPipeline->GetDescriptorSetLayouts()[0];

	uniformBuffers = CreateUniformBuffers(vulkanContext, maxFramesInFlight);
	descriptorPool = CreateDescriptorPool(maxFramesInFlight, maxFramesInFlight, device);
	descriptorSets = CreateDescriptorSets(descriptorPool, layout, maxFramesInFlight, device);	

	PopulateDescriptorSets(descriptorSets, uniformBuffers, scene.GetImageView(), scene.GetSampler(), device);

	eventSystem.Subscribe<ES::WindowResized>(this, &RenderSystem::OnResize);
}

RenderSystem::~RenderSystem()
{
	eventSystem.Unsubscribe<ES::WindowResized>(this);

	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();
	
	RenderSystemDetails::DestroyFramebuffers(framebuffers, device);
	VulkanHelpers::DestroyImageView(device, depthImageView);

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void RenderSystem::Process(float deltaSeconds)
{
	// Let this code be here for now
	constexpr float rotationRate = 20.0f;
	static float totalAngle = 0.0f;

	totalAngle = std::fmod(totalAngle + deltaSeconds * rotationRate, 360.0f);

	ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(totalAngle), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	VkExtent2D extent = vulkanContext.GetSwapchain().GetExtent();
	ubo.projection = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 10.0f);
	ubo.projection[1][1] *= -1;
}

void RenderSystem::Render()
{
	using namespace RenderSystemDetails;
	using namespace VulkanHelpers;

	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();
	const VkSwapchainKHR swapchain = vulkanContext.GetSwapchain().GetVkSwapchainKHR();
	const auto& [waitSemaphores, waitStages, signalSemaphores, fence] = syncs[currentFrame].AsTuple();

	vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &fence);
	
	uint32_t imageIndex;
	const VkResult acquireResult = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), 
		waitSemaphores[0], VK_NULL_HANDLE, &imageIndex);
	Assert(acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR);

	uniformBuffers[currentFrame].Fill(std::span{ reinterpret_cast<const std::byte*>(&ubo), sizeof(ubo) });

	VkCommandBuffer commandBuffer = commandBuffers[currentFrame];
	const Queues& queues = vulkanContext.GetDevice().GetQueues();

	const auto renderCommands = [&](VkCommandBuffer buffer) {
		EnqueueRenderCommands(vulkanContext, buffer, framebuffers[imageIndex], *renderPass, *graphicsPipeline, scene,
			std::span{ &descriptorSets[currentFrame], 1 });
	};

	VulkanHelpers::SubmitCommandBuffer(commandBuffer, queues.graphics, renderCommands, syncs[currentFrame]);

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores.data();

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

	const Device& device = vulkanContext.GetDevice();
	device.WaitIdle();

	DestroyFramebuffers(framebuffers, device.GetVkDevice());
	VulkanHelpers::DestroyImageView(device.GetVkDevice(), depthImageView);

	Swapchain& swapchain = vulkanContext.GetSwapchain();

	if (event.newWidth != 0 && event.newHeight != 0)
	{
		swapchain.Recreate({event.newWidth, event.newHeight});
	}

	depthAttachment = CreateDepthAttachment(swapchain.GetExtent(), vulkanContext);
	depthImageView = depthAttachment->CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
	framebuffers = CreateFramebuffers(swapchain, depthImageView, renderPass->GetVkRenderPass(), device.GetVkDevice());
}
#include "Engine/Systems/RenderSystem.hpp"

#include "Engine/Engine.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Resources/ImageView.hpp"
#include "Shaders/Common.h"

#include <glm/gtc/matrix_transform.hpp>

namespace RenderSystemDetails
{
	static std::vector<VkFramebuffer> CreateFramebuffers(const Swapchain& swapchain, const ImageView& colorAttachmentView, 
		const ImageView& depthAttachmentView, VkRenderPass renderPass, VkDevice device)
	{
		std::vector<VkFramebuffer> framebuffers;
		framebuffers.reserve(swapchain.GetImageViews().size());

		const auto createFramebuffer = [&](const ImageView& imageView) {
			std::array<VkImageView, 3> attachments = { colorAttachmentView.GetVkImageView(), 
				depthAttachmentView.GetVkImageView(), imageView.GetVkImageView() };

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

	static std::unique_ptr<Image> CreateColorAttachment(VkExtent2D extent, const VulkanContext& vulkanContext)
	{
		ImageDescription imageDescription{
			.extent = { static_cast<int>(extent.width), static_cast<int>(extent.height) },
			.mipLevelsCount = 1,
			.samples = vulkanContext.GetDevice().GetMaxSampleCount(),
			.format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
			.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

		return std::make_unique<Image>(imageDescription, &vulkanContext);
	}

	static std::unique_ptr<Image> CreateDepthAttachment(VkExtent2D extent, const VulkanContext& vulkanContext)
	{
		ImageDescription imageDescription{ 
			.extent = { static_cast<int>(extent.width), static_cast<int>(extent.height) },
			.mipLevelsCount = 1,
			.samples = vulkanContext.GetDevice().GetMaxSampleCount(),
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

	static void DrawSceneNode(const SceneNode& node, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
	{
		if (!node.visible)
		{
			return;
		}

		if (!node.mesh.primitives.empty()) 
		{
			glm::mat4 nodeTransform = node.transform;

			SceneNode* currentParent = node.parent;

			while (currentParent) 
			{
				nodeTransform = currentParent->transform * nodeTransform;
				currentParent = currentParent->parent;
			}

			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeTransform);

			std::ranges::for_each(node.mesh.primitives, [&](const auto& primitive) {
				if (primitive.indexCount > 0)
				{
					vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
				}
			});
		}

		std::ranges::for_each(node.children, [&](const auto& childNode) {
			DrawSceneNode(*childNode, commandBuffer, pipelineLayout);
		});
	}

	static void EnqueueRenderCommands(const VulkanContext& vulkanContext, VkCommandBuffer commandBuffer, 
		VkFramebuffer framebuffer, const RenderPass& renderPass, const GraphicsPipeline& graphicsPipeline, 
		const Scene& scene, const std::span<const VkDescriptorSet> descriptorSets)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass.GetVkRenderPass();
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = vulkanContext.GetSwapchain().GetExtent();

		std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { { 0.73f, 0.95f, 1.0f, 1.0f } };
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

		VkBuffer vertexBuffers[] = { scene.GetVertexBuffer().GetVkBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, scene.GetIndexBuffer().GetVkBuffer(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipelineLayout(), 
			0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

		DrawSceneNode(scene.GetRoot(), commandBuffer, graphicsPipeline.GetPipelineLayout());

		vkCmdEndRenderPass(commandBuffer);
	}

	static void DestroyFramebuffers(std::vector<VkFramebuffer>& framebuffers, VkDevice device)
	{
		std::ranges::for_each(framebuffers, [=](VkFramebuffer framebuffer) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		});

		framebuffers.clear();
	}

	static std::vector<Buffer> CreateUniformBuffers(const VulkanContext& vulkanContext, const size_t count)
	{
		const auto createBuffer = [&]() {
			BufferDescription bufferDescription{ static_cast<VkDeviceSize>(sizeof(gpu::UniformBufferObject)),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

			auto result = Buffer(bufferDescription, true, &vulkanContext);
			std::ignore = result.GetStagingBuffer()->MapMemory(true);

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
		const ImageView& imageView, VkSampler sampler, VkDevice device)
	{
		Assert(sets.size() == buffers.size());

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imageView.GetVkImageView();
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

RenderSystem::RenderSystem(EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
	: vulkanContext{aVulkanContext}
	, eventSystem{aEventSystem}
	, renderPass{std::make_unique<RenderPass>(vulkanContext)}
	, graphicsPipeline{std::make_unique<GraphicsPipeline>(*renderPass, vulkanContext)}
{
	using namespace RenderSystemDetails;
	using namespace VulkanHelpers;
	using namespace VulkanConfig;

	const VkCommandPool longLivedPool = vulkanContext.GetDevice().GetCommandPool(CommandBufferType::eLongLived);
	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();

	CreateAttachmentsAndFramebuffers();

	commandBuffers = CreateCommandBuffers(device, maxFramesInFlight, longLivedPool);
	syncs = CreateCommandBufferSyncs(maxFramesInFlight, device);

	VkDescriptorSetLayout layout = graphicsPipeline->GetDescriptorSetLayouts()[0];

	uniformBuffers = CreateUniformBuffers(vulkanContext, maxFramesInFlight);
	descriptorPool = CreateDescriptorPool(maxFramesInFlight, maxFramesInFlight, device);
	descriptorSets = CreateDescriptorSets(descriptorPool, layout, maxFramesInFlight, device);

	eventSystem.Subscribe<ES::WindowResized>(this, &RenderSystem::OnResize);
	eventSystem.Subscribe<ES::SceneOpened>(this, &RenderSystem::OnSceneOpen);
	eventSystem.Subscribe<ES::BeforeWindowRecreated>(this, &RenderSystem::OnBeforeWindowRecreated, ES::Priority::eHigh);
	eventSystem.Subscribe<ES::WindowRecreated>(this, &RenderSystem::OnWindowRecreated, ES::Priority::eLow);
}

RenderSystem::~RenderSystem()
{
	eventSystem.Unsubscribe<ES::WindowResized>(this);
	eventSystem.Unsubscribe<ES::SceneOpened>(this);
	eventSystem.Unsubscribe<ES::BeforeWindowRecreated>(this);
	eventSystem.Unsubscribe<ES::WindowRecreated>(this);

	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();

	DestroyAttachmentsAndFramebuffers();

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void RenderSystem::Process(float deltaSeconds)
{
	if (!scene)
	{
		return;
	}

	const CameraComponent& camera = scene->GetCamera();

	ubo.view = camera.GetViewMatrix();
	ubo.projection = camera.GetProjectionMatrix();
	ubo.viewPos = camera.GetPosition();
}

void RenderSystem::Render()
{
	using namespace RenderSystemDetails;
	using namespace VulkanHelpers;

	if (!scene)
	{
		return;
	}

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
		EnqueueRenderCommands(vulkanContext, buffer, framebuffers[imageIndex], *renderPass, *graphicsPipeline, *scene,
			std::span{ &descriptorSets[currentFrame], 1 });
	};

	SubmitCommandBuffer(commandBuffer, queues.graphics, renderCommands, syncs[currentFrame]);

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
	if (event.newExtent.width == 0 || event.newExtent.height == 0)
	{
		return;
	}

	DestroyAttachmentsAndFramebuffers();
	vulkanContext.GetSwapchain().Recreate(event.newExtent);
	CreateAttachmentsAndFramebuffers();
}

void RenderSystem::OnBeforeWindowRecreated(const ES::BeforeWindowRecreated& event)
{
	DestroyAttachmentsAndFramebuffers();
}

void RenderSystem::OnWindowRecreated(const ES::WindowRecreated& event)
{
	CreateAttachmentsAndFramebuffers();
}

void RenderSystem::OnSceneOpen(const ES::SceneOpened& event)
{
	using namespace RenderSystemDetails;

	scene = &event.scene;

	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();
	PopulateDescriptorSets(descriptorSets, uniformBuffers, scene->GetImageView(), scene->GetSampler(), device);
}

void RenderSystem::CreateAttachmentsAndFramebuffers()
{
	using namespace RenderSystemDetails;

	const Device& device = vulkanContext.GetDevice();
	device.WaitIdle();

	const Swapchain& swapchain = vulkanContext.GetSwapchain();

	colorAttachment = CreateColorAttachment(swapchain.GetExtent(), vulkanContext);
	colorAttachmentView = std::make_unique<ImageView>(*colorAttachment, VK_IMAGE_ASPECT_COLOR_BIT, &vulkanContext);

	depthAttachment = CreateDepthAttachment(swapchain.GetExtent(), vulkanContext);
	depthAttachmentView = std::make_unique<ImageView>(*depthAttachment, VK_IMAGE_ASPECT_DEPTH_BIT, &vulkanContext);

	framebuffers = CreateFramebuffers(swapchain, *colorAttachmentView, *depthAttachmentView,
		renderPass->GetVkRenderPass(), device.GetVkDevice());
}

void RenderSystem::DestroyAttachmentsAndFramebuffers()
{
	using namespace RenderSystemDetails;

	const Device& device = vulkanContext.GetDevice();
	device.WaitIdle();

	DestroyFramebuffers(framebuffers, device.GetVkDevice());

	colorAttachmentView.reset();
	colorAttachment.reset();

	depthAttachmentView.reset();
	depthAttachment.reset();
}
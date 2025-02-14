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
    static std::vector<Frame> CreateFrames(const size_t count, const VulkanContext& vulkanContext)
    {
        std::vector<Frame> frames(count);
        
        std::ranges::generate(frames, [&]() {
            return Frame(&vulkanContext);
        });
        
        return frames;
    }

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

    static void EnqueueRenderCommands(const VulkanContext& vulkanContext, const Frame& frame,
        VkFramebuffer framebuffer, const RenderPass& renderPass, const GraphicsPipeline& graphicsPipeline,
        const Scene& scene, const Buffer& indirectBuffer, const uint32_t indirectDrawCount)
    {
        const VkCommandBuffer commandBuffer = frame.GetCommandBuffer();
        const VkDescriptorSet descriptorSet = frame.GetDescriptorSet();
        
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
            0, 1, &descriptorSet, 0, nullptr);

        vkCmdDrawIndexedIndirect(commandBuffer, indirectBuffer.GetVkBuffer(), 0, indirectDrawCount, 
            sizeof(VkDrawIndexedIndirectCommand));

        vkCmdEndRenderPass(commandBuffer);
    }

    static void DestroyFramebuffers(std::vector<VkFramebuffer>& framebuffers, VkDevice device)
    {
        std::ranges::for_each(framebuffers, [=](VkFramebuffer framebuffer) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        });

        framebuffers.clear();
    }

    static VkDescriptorPool CreateDescriptorPool(const uint32_t descriptorCount, const uint32_t maxSets, VkDevice device)
    {
        VkDescriptorPool descriptorPool;

        std::array<VkDescriptorPoolSize, 3> poolSizes{};

        VkDescriptorPoolSize& uniformBufferPoolSize = poolSizes[0];
        uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferPoolSize.descriptorCount = descriptorCount;

        VkDescriptorPoolSize& samplersPoolSize = poolSizes[1];
        samplersPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplersPoolSize.descriptorCount = descriptorCount;

        VkDescriptorPoolSize& SSBOPoolSize = poolSizes[2];
        SSBOPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        SSBOPoolSize.descriptorCount = descriptorCount;
        
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = maxSets;

        const VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
        Assert(result == VK_SUCCESS);

        return descriptorPool;
    }

    static std::tuple<std::unique_ptr<Buffer>, uint32_t> CreateIndirectBuffer(const Scene& scene, 
        const VulkanContext& vulkanContext)
    {
        std::vector<VkDrawIndexedIndirectCommand> indirectCommands;

        const auto createDrawCommand = [&](const Primitive& primitive) {
            VkDrawIndexedIndirectCommand indirectCommand{};
            indirectCommand.instanceCount = 1;
            indirectCommand.firstIndex = primitive.firstIndex;
            indirectCommand.indexCount = primitive.indexCount;
            indirectCommand.firstInstance = primitive.nodeId; // Using this to access transforms SSBO in shaders

            return indirectCommand;
        };

        std::ranges::for_each(scene.GetNodes(), [&](const SceneNode* node) {

            if (!node->visible)
            {
                return;
            }

            std::ranges::transform(node->mesh.primitives, std::back_inserter(indirectCommands), createDrawCommand);
        });

        std::span indirectCommandsSpan(std::as_const(indirectCommands));

        BufferDescription indirectBufferDescription{ .size = static_cast<VkDeviceSize>(indirectCommandsSpan.size_bytes()),
            .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        return { std::make_unique<Buffer>(indirectBufferDescription, true, indirectCommandsSpan, &vulkanContext),
            static_cast<uint32_t>(indirectCommands.size()) };
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
    
    const VkDevice device = vulkanContext.GetDevice().GetVkDevice();

    CreateAttachmentsAndFramebuffers();
    
    frames = CreateFrames(maxFramesInFlight, vulkanContext);

    descriptorPool = CreateDescriptorPool(maxFramesInFlight, maxFramesInFlight, device);

    eventSystem.Subscribe<ES::WindowResized>(this, &RenderSystem::OnResize);
    eventSystem.Subscribe<ES::SceneOpened>(this, &RenderSystem::OnSceneOpen);
    eventSystem.Subscribe<ES::SceneClosed>(this, &RenderSystem::OnSceneClose);
    eventSystem.Subscribe<ES::BeforeWindowRecreated>(this, &RenderSystem::OnBeforeWindowRecreated, ES::Priority::eHigh);
    eventSystem.Subscribe<ES::WindowRecreated>(this, &RenderSystem::OnWindowRecreated, ES::Priority::eLow);
}

RenderSystem::~RenderSystem()
{
    eventSystem.Unsubscribe<ES::WindowResized>(this);
    eventSystem.Unsubscribe<ES::SceneOpened>(this);
    eventSystem.Unsubscribe<ES::SceneClosed>(this);
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
    
    Frame& frame = frames[currentFrame];

    const VkDevice device = vulkanContext.GetDevice().GetVkDevice();
    const VkSwapchainKHR swapchain = vulkanContext.GetSwapchain().GetVkSwapchainKHR();
    const auto& [waitSemaphores, waitStages, signalSemaphores, fence] = frame.GetSync().AsTuple();

    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &fence);
    
    uint32_t imageIndex;
    const VkResult acquireResult = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), 
        waitSemaphores[0], VK_NULL_HANDLE, &imageIndex);
    Assert(acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR);

    frame.GetUniformBuffer().Fill(std::span{ reinterpret_cast<const std::byte*>(&ubo), sizeof(ubo) });

    VkCommandBuffer commandBuffer = frame.GetCommandBuffer();
    const Queues& queues = vulkanContext.GetDevice().GetQueues();

    const auto renderCommands = [&](VkCommandBuffer buffer) {
        EnqueueRenderCommands(vulkanContext, frame, framebuffers[imageIndex], *renderPass, *graphicsPipeline, *scene,
            *indirectBuffer, indirectDrawCount);
    };

    SubmitCommandBuffer(commandBuffer, queues.graphics, renderCommands, frame.GetSync());

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
    using namespace VulkanConfig;
    using namespace VulkanHelpers;

    scene = &event.scene;

    const VkDevice device = vulkanContext.GetDevice().GetVkDevice();
    VkDescriptorSetLayout layout = graphicsPipeline->GetDescriptorSetLayouts()[0];
    
    std::vector<VkDescriptorSet> descriptorSets = CreateDescriptorSets(descriptorPool, layout, maxFramesInFlight, device);
    
    for (size_t i = 0; i < maxFramesInFlight; ++i)
    {
        PopulateDescriptorSet(descriptorSets[i], frames[i].GetUniformBuffer(), *scene, device);
        frames[i].SetDescriptorSet(descriptorSets[i]);
    }

    std::tie(indirectBuffer, indirectDrawCount) = CreateIndirectBuffer(*scene, vulkanContext);
}

void RenderSystem::OnSceneClose(const ES::SceneClosed& event)
{
    const Device& device = vulkanContext.GetDevice();
    
    device.WaitIdle();
    
    for (Frame& frame : frames)
    {
        frame.SetDescriptorSet(VK_NULL_HANDLE);
    }
    
    vkResetDescriptorPool(device.GetVkDevice(), descriptorPool, 0);
    
    scene = nullptr;
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

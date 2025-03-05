#include "Engine/Render/SceneRenderer.hpp"

#include "Engine/EventSystem.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipelineBuilder.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace SceneRendererDetails
{
    static constexpr std::string_view vertexShaderPath = "~/Shaders/shader.vert";
    static constexpr std::string_view fragmentShaderPath = "~/Shaders/shader.frag";

    static std::vector<ShaderModule> GetShaderModules(const ShaderManager& shaderManager)
    {
        std::vector<ShaderModule> shaderModules;
        shaderModules.reserve(2);

        shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(vertexShaderPath), ShaderType::eVertex));
        shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(fragmentShaderPath), ShaderType::eFragment));

        return shaderModules;
    }

    static RenderPass CreateRenderPass(const VulkanContext& vulkanContext)
    {
        AttachmentDescription colorAttachmentDescription = {
            .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .actualLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        AttachmentDescription resolveAttachmentDescription = {
            .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .actualLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        AttachmentDescription depthStencilAttachmentDescription = {
            .format = VulkanConfig::depthImageFormat,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .actualLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

        return RenderPassBuilder(vulkanContext)
            .SetBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
            .SetMultisampling(vulkanContext.GetDevice().GetMaxSampleCount())
            .AddColorAndResolveAttachments(colorAttachmentDescription, resolveAttachmentDescription)
            .AddDepthStencilAttachment(depthStencilAttachmentDescription)
            .Build();
    }

    static Buffer CreateUniformBuffer(const VulkanContext& vulkanContext)
    {
        BufferDescription bufferDescription{ sizeof(gpu::UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

        return { bufferDescription, false, vulkanContext };
    }

    static std::tuple<std::vector<VkDescriptorSet>, DescriptorSetLayout> CreateDescriptors(
        const std::vector<Buffer>& uniformBuffers, const VulkanContext& vulkanContext)
    {
        DescriptorSetLayout layout = vulkanContext.GetDescriptorSetsManager().GetDescriptorSetLayoutBuilder()
            .AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .Build();

        std::vector<VkDescriptorSet> descriptors;

        std::ranges::transform(uniformBuffers, std::back_inserter(descriptors), [&](const Buffer& uniformBuffer) {
            return std::get<0>(vulkanContext.GetDescriptorSetsManager().GetDescriptorSetBuilder(layout)
            .Bind(0, uniformBuffer)
            .Build());
        });

        return { descriptors, layout };
    }
}

SceneRenderer::SceneRenderer(EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , eventSystem{ &aEventSystem }
{
    using namespace SceneRendererDetails;

    renderPass = CreateRenderPass(*vulkanContext);

    CreateRenderTargetsAndFramebuffers();

    uniformBuffers.resize(VulkanConfig::maxFramesInFlight);
    std::ranges::generate(uniformBuffers, [&]() { return CreateUniformBuffer(*vulkanContext); });

    std::tie(uniformDescriptors, layout) = CreateDescriptors(uniformBuffers, *vulkanContext);

    eventSystem->Subscribe<ES::BeforeSwapchainRecreated>(this, &SceneRenderer::OnBeforeSwapchainRecreated);
    eventSystem->Subscribe<ES::SwapchainRecreated>(this, &SceneRenderer::OnSwapchainRecreated);
    eventSystem->Subscribe<ES::TryReloadShaders>(this, &SceneRenderer::OnTryReloadShaders);
    eventSystem->Subscribe<ES::SceneOpened>(this, &SceneRenderer::OnSceneOpen);
    eventSystem->Subscribe<ES::SceneClosed>(this, &SceneRenderer::OnSceneClose);
}

SceneRenderer::~SceneRenderer()
{
    eventSystem->UnsubscribeAll(this);

    DestroyRenderTargetsAndFramebuffers();
}

void SceneRenderer::Process(const float deltaSeconds)
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

void SceneRenderer::Render(const Frame& frame)
{
    using namespace VulkanUtils;

    if (!scene)
    {
        return;
    }

    uniformBuffers[frame.index].Fill(std::as_bytes(std::span(&ubo, 1)));

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[frame.swapchainRenderTargetIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vulkanContext->GetSwapchain().GetExtent();

    std::array<VkClearValue, 3> clearValues{};
    clearValues[0].color = { { 0.73f, 0.95f, 1.0f, 1.0f } };
    clearValues[2].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    const VkCommandBuffer commandBuffer = frame.commandBuffer;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    const VkExtent2D extent = vulkanContext->GetSwapchain().GetExtent();
    VkViewport viewport = GetViewport(static_cast<float>(extent.width), static_cast<float>(extent.height));
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    const VkRect2D scissor = GetScissor(extent);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { scene->GetVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, scene->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    std::vector<VkDescriptorSet> descriptors = { uniformDescriptors[frame.index] };
    std::ranges::copy(scene->GetGlobalDescriptors(), std::back_inserter(descriptors));

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetLayout(),
        0, static_cast<uint32_t>(descriptors.size()), descriptors.data(), 0, nullptr);

    vkCmdDrawIndexedIndirect(commandBuffer, *indirectBuffer, 0, indirectDrawCount, sizeof(VkDrawIndexedIndirectCommand));

    vkCmdEndRenderPass(commandBuffer);
}

void SceneRenderer::CreateGraphicsPipeline(std::vector<ShaderModule>&& shaderModules)
{
    using namespace VulkanUtils;

    std::vector<VkDescriptorSetLayout> layouts = { layout, Scene::GetGlobalDescriptorSetLayout(*vulkanContext) };

    graphicsPipeline = GraphicsPipelineBuilder(*vulkanContext)
        .SetDescriptorSetLayouts(std::move(layouts))
        .AddPushConstantRange({ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants) })
        .SetShaderModules(std::move(shaderModules))
        .SetVertexData(Vertex::GetBindings(), Vertex::GetAttributes())
        .SetInputTopology(InputTopology::eTriangleList)
        .SetPolygonMode(PolygonMode::eFill)
        .SetCullMode(CullMode::eBack, false)
        .SetMultisampling(vulkanContext->GetDevice().GetMaxSampleCount())
        .SetDepthState(true, true, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(renderPass)
        .Build();
}

void SceneRenderer::CreateRenderTargetsAndFramebuffers()
{
    const Swapchain& swapchain = vulkanContext->GetSwapchain();
    const VkExtent2D swapchainExtent = swapchain.GetExtent();

    // Render targets
    ImageDescription colorTargetDescription = {
        .extent = { swapchainExtent.width, swapchainExtent.height, 1 },
        .mipLevelsCount = 1,
        .samples = vulkanContext->GetDevice().GetMaxSampleCount(),
        .format = swapchain.GetSurfaceFormat().format,
        .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    
    ImageDescription depthTargetDescription = {
        .extent = { swapchainExtent.width, swapchainExtent.height, 1 },
        .mipLevelsCount = 1,
        .samples = vulkanContext->GetDevice().GetMaxSampleCount(),
        .format = VulkanConfig::depthImageFormat,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    
    colorTarget = RenderTarget(colorTargetDescription, VK_IMAGE_ASPECT_COLOR_BIT, *vulkanContext);
    depthTarget = RenderTarget(depthTargetDescription, VK_IMAGE_ASPECT_DEPTH_BIT, *vulkanContext);

    // Framebuffers
    const std::vector<RenderTarget>& swapchainTargets = swapchain.GetRenderTargets();
    framebuffers.reserve(swapchainTargets.size());

    std::vector<VkImageView> attachments = { colorTarget.view, VK_NULL_HANDLE, depthTarget.view };

    std::ranges::transform(swapchainTargets, std::back_inserter(framebuffers), [&](const RenderTarget& target) {
        attachments[1] = target.view;
        return VulkanUtils::CreateFrameBuffer(renderPass, swapchain.GetExtent(), attachments, *vulkanContext);
    });
}

void SceneRenderer::DestroyRenderTargetsAndFramebuffers()
{
    using namespace VulkanUtils;

    DestroyFramebuffers(framebuffers, *vulkanContext);

    colorTarget.~RenderTarget();
    depthTarget.~RenderTarget();
}

void SceneRenderer::OnBeforeSwapchainRecreated(const ES::BeforeSwapchainRecreated& event)
{
    DestroyRenderTargetsAndFramebuffers();
}

void SceneRenderer::OnSwapchainRecreated(const ES::SwapchainRecreated& event)
{
    CreateRenderTargetsAndFramebuffers();
}

void SceneRenderer::OnTryReloadShaders(const ES::TryReloadShaders& event)
{
    using namespace SceneRendererDetails;

    if (std::vector<ShaderModule> shaderModules = GetShaderModules(vulkanContext->GetShaderManager());
        std::ranges::all_of(shaderModules, &ShaderModule::IsValid))
    {
        CreateGraphicsPipeline(std::move(shaderModules));
    }
}

void SceneRenderer::OnSceneOpen(const ES::SceneOpened& event)
{
    using namespace SceneRendererDetails;
    using namespace VulkanUtils;

    scene = &event.scene;

    if (!graphicsPipeline.IsValid())
    {
        std::vector<ShaderModule> shaderModules = GetShaderModules(vulkanContext->GetShaderManager());

        const bool allValid = std::ranges::all_of(shaderModules, &ShaderModule::IsValid);
        Assert(allValid);

        CreateGraphicsPipeline(std::move(shaderModules));
    }

    std::tie(indirectBuffer, indirectDrawCount) = CreateIndirectBuffer(*scene, *vulkanContext);
}

void SceneRenderer::OnSceneClose(const ES::SceneClosed& event)
{
    scene = nullptr;
}

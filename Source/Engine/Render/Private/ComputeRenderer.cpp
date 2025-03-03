#include "Engine/Render/ComputeRenderer.hpp"

#include "Utils/Constants.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Resources/ImageView.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ImageUtils.hpp"
#include "Engine/Render/Vulkan/Resources/Pipelines/ComputePipelineBuilder.hpp"

namespace ComputeRendererDetails
{
    static constexpr std::string_view shaderPath = "~/Shaders/gradient.comp";
}

ComputeRenderer::ComputeRenderer(EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , eventSystem{ &aEventSystem }
{
    using namespace ComputeRendererDetails;

    CreateRenderTarget();
    
    const ShaderManager& shaderManager = vulkanContext->GetShaderManager();
    
    ShaderModule shaderModule = shaderManager.CreateShaderModule(FilePath(shaderPath), ShaderType::eCompute);
    Assert(shaderModule.IsValid());

    CreatePipeline(std::move(shaderModule));

    eventSystem->Subscribe<ES::BeforeSwapchainRecreated>(this, &ComputeRenderer::OnBeforeSwapchainRecreated);
    eventSystem->Subscribe<ES::SwapchainRecreated>(this, &ComputeRenderer::OnSwapchainRecreated);
    eventSystem->Subscribe<ES::TryReloadShaders>(this, &ComputeRenderer::OnTryReloadShaders);
}

ComputeRenderer::~ComputeRenderer()
{
    eventSystem->UnsubscribeAll(this);

    DestroyRenderTarget();
}

void ComputeRenderer::Process(const float deltaSeconds)
{
    
}

void ComputeRenderer::Render(const Frame& frame)
{
    using namespace ImageUtils;
    
    const VkCommandBuffer commandBuffer = frame.commandBuffer;
    const Image& swapchainImage = vulkanContext->GetSwapchain().GetImages()[frame.swapchainImageIndex];
    const VkExtent3D targetExtent = renderTarget->GetDescription().extent;
    
    TransitionLayout(commandBuffer, *renderTarget, LayoutTransitions::undefinedToGeneral, {
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT });
    
    FillImage(commandBuffer, *renderTarget, Color::magenta);
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.Get());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.GetLayout(), 0, 1, &descriptor, 0, nullptr);
    
    vkCmdDispatch(commandBuffer, std::ceil(targetExtent.width / 16.0), std::ceil(targetExtent.height / 16.0), 1);
    
    TransitionLayout(commandBuffer, *renderTarget, LayoutTransitions::generalToSrcOptimal, {
        .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT });
    
    TransitionLayout(commandBuffer, swapchainImage, LayoutTransitions::undefinedToDstOptimal, {
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT });
    
    BlitImageToImage(commandBuffer, *renderTarget, swapchainImage);
    
    TransitionLayout(commandBuffer, swapchainImage, LayoutTransitions::dstOptimalToColorAttachmentOptimal, {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT });
}

void ComputeRenderer::CreatePipeline(ShaderModule&& shaderModule)
{
    using namespace VulkanUtils;

    std::vector<VkDescriptorSetLayout> layouts = { layout.Get() };
    
    computePipeline = ComputePipelineBuilder(*vulkanContext)
        .SetDescriptorSetLayouts(std::move(layouts))
        .SetShaderModule(std::move(shaderModule))
        .Build();
}

void ComputeRenderer::CreateRenderTarget()
{
    using namespace VulkanUtils;
    
    // Let this target match window size
    const VkExtent2D extent = vulkanContext->GetSwapchain().GetExtent();
    
    ImageDescription imageDescription{
        .extent = { extent.width, extent.height, 1 },
        .mipLevelsCount = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    // TODO: Render target class? Add layout inside to track it?
    renderTarget = std::make_unique<Image>(imageDescription, *vulkanContext);
    renderTargetView = std::make_unique<ImageView>(*renderTarget, VK_IMAGE_ASPECT_COLOR_BIT, *vulkanContext);
    
    DescriptorSetManager& descriptorSetManager = vulkanContext->GetDescriptorSetsManager();
    
    descriptorSetManager.ResetDescriptors(DescriptorScope::eComputeRenderer);
    
    std::tie(descriptor, layout) = descriptorSetManager.GetDescriptorSetBuilder(DescriptorScope::eComputeRenderer)
        .Bind(0, *renderTargetView, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .Build();
}

void ComputeRenderer::DestroyRenderTarget()
{
    using namespace VulkanUtils;
    
    renderTargetView.reset();
    renderTarget.reset();
}

void ComputeRenderer::OnBeforeSwapchainRecreated(const ES::BeforeSwapchainRecreated& event)
{
    DestroyRenderTarget();
}

void ComputeRenderer::OnSwapchainRecreated(const ES::SwapchainRecreated& event)
{
    CreateRenderTarget();
}

void ComputeRenderer::OnTryReloadShaders(const ES::TryReloadShaders& event)
{
    using namespace ComputeRendererDetails;
    
    const ShaderManager& shaderManager = vulkanContext->GetShaderManager();

    if (ShaderModule shaderModule = shaderManager.CreateShaderModule(FilePath(shaderPath), ShaderType::eCompute);
        shaderModule.IsValid())
    {
        CreatePipeline(std::move(shaderModule));
    }
}

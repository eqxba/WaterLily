#include "Engine/Render/ComputeRenderer.hpp"

#include "Utils/Constants.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Image/ImageUtils.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipelineBuilder.hpp"

namespace ComputeRendererDetails
{
    static constexpr std::string_view shaderPath = "~/Shaders/Gradient.comp";

    static RenderTarget CreateRenderTarget(const VulkanContext& vulkanContext)
    {
        const VkExtent2D extent = vulkanContext.GetSwapchain().GetExtent();
        
        ImageDescription description = {
            .extent = { extent.width, extent.height, 1 },
            .mipLevelsCount = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
        
        auto renderTarget = RenderTarget(description, VK_IMAGE_ASPECT_COLOR_BIT, vulkanContext);
        
        vulkanContext.GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer cmd) {
            ImageUtils::TransitionLayout(cmd, renderTarget, LayoutTransitions::undefinedToDstOptimal, Barriers::noneToTransferWrite);
        });
        
        return renderTarget;
    }
}

ComputeRenderer::ComputeRenderer(EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , eventSystem{ &aEventSystem }
{
    using namespace ComputeRendererDetails;

    renderTarget = CreateRenderTarget(*vulkanContext);
    
    const ShaderManager& shaderManager = vulkanContext->GetShaderManager();
    
    ShaderModule shaderModule = shaderManager.CreateShaderModule(FilePath(shaderPath), VK_SHADER_STAGE_COMPUTE_BIT, {});
    Assert(shaderModule.IsValid());

    CreatePipeline(shaderModule);
    CreateDescriptors();

    eventSystem->Subscribe<ES::BeforeSwapchainRecreated>(this, &ComputeRenderer::OnBeforeSwapchainRecreated);
    eventSystem->Subscribe<ES::SwapchainRecreated>(this, &ComputeRenderer::OnSwapchainRecreated);
    eventSystem->Subscribe<ES::TryReloadShaders>(this, &ComputeRenderer::OnTryReloadShaders);
}

ComputeRenderer::~ComputeRenderer()
{
    eventSystem->UnsubscribeAll(this);
    
    vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eComputeRenderer);
}

void ComputeRenderer::Process(const Frame& frame, const float deltaSeconds)
{}

void ComputeRenderer::Render(const Frame& frame)
{
    using namespace ImageUtils;
    
    const VkCommandBuffer commandBuffer = frame.commandBuffer;
    const RenderTarget& swapchainTarget = vulkanContext->GetSwapchain().GetRenderTargets()[frame.swapchainImageIndex];
    const VkExtent3D targetExtent = renderTarget.image.GetDescription().extent;
    
    FillImage(commandBuffer, renderTarget, Color::magenta);
    
    TransitionLayout(commandBuffer, renderTarget, LayoutTransitions::dstOptimalToGeneral, Barriers::transferWriteToComputeRead);
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.GetLayout(), 0,
        static_cast<uint32_t>(descriptors.size()), descriptors.data(), 0, nullptr);

    const VkExtent3D groupCount = { static_cast<uint32_t>(std::ceil(targetExtent.width / 16.0)),
        static_cast<uint32_t>(std::ceil(targetExtent.height / 16.0)), 1 };

    vkCmdDispatch(commandBuffer, groupCount.width, groupCount.height, groupCount.depth);
    
    TransitionLayout(commandBuffer, renderTarget, LayoutTransitions::generalToSrcOptimal, Barriers::computeWriteToTransferRead);
    TransitionLayout(commandBuffer, swapchainTarget, LayoutTransitions::undefinedToDstOptimal, Barriers::noneToTransferWrite);
    
    BlitImageToImage(commandBuffer, renderTarget, swapchainTarget);

    TransitionLayout(commandBuffer, renderTarget, LayoutTransitions::srcOptimalToDstOptimal, Barriers::transferReadToTransferWrite);
    TransitionLayout(commandBuffer, swapchainTarget, LayoutTransitions::dstOptimalToColorAttachmentOptimal, Barriers::transferWriteToColorReadWrite);
}

void ComputeRenderer::CreatePipeline(const ShaderModule& shaderModule)
{
    computePipeline = ComputePipelineBuilder(*vulkanContext)
        .SetShaderModule(shaderModule)
        .Build();
}

void ComputeRenderer::CreateDescriptors()
{
    Assert(descriptors.empty());
    
    descriptors = vulkanContext->GetDescriptorSetsManager()
        .GetReflectiveDescriptorSetBuilder(computePipeline, DescriptorScope::eComputeRenderer)
        .Bind("renderTarget", renderTarget, 0, VK_IMAGE_LAYOUT_GENERAL)
        .Build();
}

void ComputeRenderer::OnBeforeSwapchainRecreated()
{
    vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eComputeRenderer);
    descriptors.clear();
}

void ComputeRenderer::OnSwapchainRecreated()
{
    using namespace ComputeRendererDetails;

    renderTarget = CreateRenderTarget(*vulkanContext);
    CreateDescriptors();
}

void ComputeRenderer::OnTryReloadShaders()
{
    using namespace ComputeRendererDetails;
    
    const ShaderManager& shaderManager = vulkanContext->GetShaderManager();

    if (ShaderModule shaderModule = shaderManager.CreateShaderModule(FilePath(shaderPath), VK_SHADER_STAGE_COMPUTE_BIT, {});
        shaderModule.IsValid())
    {
        vulkanContext->GetDevice().WaitIdle();
        vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eComputeRenderer);
        descriptors.clear();
        
        CreatePipeline(std::move(shaderModule));
        CreateDescriptors();
    }
}

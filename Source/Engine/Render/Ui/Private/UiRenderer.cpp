#include "Engine/Render/Ui/UiRenderer.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#include "Engine/Window.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Render/Ui/StatsWidget.hpp"
#include "Engine/Render/Ui/SettingsWidget.hpp"
#include "Engine/Render/Vulkan/VulkanUtils.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Image/ImageUtils.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineUtils.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipelineBuilder.hpp"

namespace UiRendererDetails
{
    static constexpr std::string_view vertexShaderPath = "~/Shaders/Ui.vert";
    static constexpr std::string_view fragmentShaderPath = "~/Shaders/Ui.frag";

    static std::tuple<Buffer, int, int> LoadFontTexture(const ImGuiIO& io, const VulkanContext& vulkanContext)
    {
        unsigned char* fontData = nullptr;
        int width = 0;
        int height = 0;

        io.Fonts->GetTexDataAsRGBA32(&fontData, &width, &height);

        const auto dataSize = static_cast<size_t>(width) * height * 4;
        const std::span<const unsigned char> fontDataSpan = { fontData, dataSize };

        const BufferDescription bufferDescription = { .size = dataSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

        return { Buffer(bufferDescription, false, fontDataSpan, vulkanContext), width, height };
    }

    static Texture CreateFontTexture(const ImGuiIO& io, const VulkanContext& vulkanContext)
    {
        using namespace ImageUtils;
        
        const auto [fontDataBuffer, width, height] = LoadFontTexture(io, vulkanContext);
         
        ImageDescription fontImageDescription = {
            .extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
            .mipLevelsCount = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
        
        SamplerDescription samplerDescription = { .addressMode = { VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE } };

        auto fontTexture = Texture(std::move(fontImageDescription), std::move(samplerDescription), vulkanContext);
        
        vulkanContext.GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer commandBuffer) {
            TransitionLayout(commandBuffer, fontTexture, LayoutTransitions::undefinedToDstOptimal, {
                .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT });

            CopyBufferToImage(commandBuffer, fontDataBuffer, fontTexture);
            
            TransitionLayout(commandBuffer, fontTexture, LayoutTransitions::dstOptimalToShaderReadOnlyOptimal, {
                .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, .dstAccessMask = VK_ACCESS_SHADER_READ_BIT });
        });

        return fontTexture;
    }

    static RenderPass CreateRenderPass(const VulkanContext& vulkanContext)
    {
        AttachmentDescription colorAttachmentDescription = {
            .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .actualLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

        return RenderPassBuilder(vulkanContext)
            .SetBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
            .AddColorAttachment(colorAttachmentDescription)
            .Build();
    }

    static std::vector<VkFramebuffer> CreateFramebuffers(const RenderPass& renderPass, const VulkanContext& vulkanContext)
    {
        using namespace VulkanUtils;
        
        const Swapchain& swapchain = vulkanContext.GetSwapchain();
        const std::vector<RenderTarget>& swapchainTargets = swapchain.GetRenderTargets();
        
        std::vector<VkFramebuffer> framebuffers;
        framebuffers.reserve(swapchainTargets.size());

        std::ranges::transform(swapchainTargets, std::back_inserter(framebuffers), [&](const RenderTarget& target) {
            const std::array<VkImageView, 1> attachments = { target.view };
            return CreateFrameBuffer(renderPass, swapchain.GetExtent(), attachments, vulkanContext);
        });
        
        return framebuffers;
    }

    static std::vector<ShaderModule> CreateShaderModules(const ShaderManager& shaderManager)
    {
        std::vector<ShaderModule> shaderModules;
        shaderModules.reserve(2);

        shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(vertexShaderPath), VK_SHADER_STAGE_VERTEX_BIT, {}));
        shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(fragmentShaderPath), VK_SHADER_STAGE_FRAGMENT_BIT, {}));

        return shaderModules;
    }

    std::vector<VkVertexInputBindingDescription> GetVertexBindings()
    {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(ImDrawVert);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return { binding };
    }

    std::vector<VkVertexInputAttributeDescription> GetVertexAttributes()
    {
        std::vector<VkVertexInputAttributeDescription> attributes(3);

        VkVertexInputAttributeDescription& posAttribute = attributes[0];
        posAttribute.binding = 0;
        posAttribute.location = 0;
        posAttribute.format = VK_FORMAT_R32G32_SFLOAT;
        posAttribute.offset = offsetof(ImDrawVert, pos);

        VkVertexInputAttributeDescription& uvAttribute = attributes[1];
        uvAttribute.binding = 0;
        uvAttribute.location = 1;
        uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
        uvAttribute.offset = offsetof(ImDrawVert, uv);

        VkVertexInputAttributeDescription& colorAttribute = attributes[2];
        colorAttribute.binding = 0;
        colorAttribute.location = 2;
        colorAttribute.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorAttribute.offset = offsetof(ImDrawVert, col);

        return attributes;
    }

    static VkRect2D GetScissor(const ImDrawCmd& command, const ImVec2& clipScale)
    {
        VkRect2D scissor;
        scissor.offset.x = std::max(static_cast<int32_t>(command.ClipRect.x * clipScale.x), 0);
        scissor.offset.y = std::max(static_cast<int32_t>(command.ClipRect.y * clipScale.y), 0);
        scissor.extent.width = static_cast<uint32_t>((command.ClipRect.z - command.ClipRect.x) * clipScale.x);
        scissor.extent.height = static_cast<uint32_t>((command.ClipRect.w - command.ClipRect.y) * clipScale.y);

        return scissor;
    }

    static void CreateWidgets(std::vector<std::unique_ptr<Widget>>& widgets, const VulkanContext& vulkanContext)
    {
        widgets.push_back(std::make_unique<StatsWidget>(vulkanContext));
        widgets.push_back(std::make_unique<SettingsWidget>(vulkanContext));
    }
}

UiRenderer::UiRenderer(const Window& aWindow, EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , eventSystem{ &aEventSystem }
    , window{ &aWindow }
    , vertexBuffers{ VulkanConfig::maxFramesInFlight }
    , indexBuffers{ VulkanConfig::maxFramesInFlight }
    , inputMode{ window->GetInputMode() }
{
    using namespace UiRendererDetails;

    ImGui::CreateContext();
    ImGui::StyleColorsClassic();
    
    // Using InitForOther intentionally to make ImGui handle all
    // of the i/o for us while implementing our own rendering
    ImGui_ImplGlfw_InitForOther(window->GetGlfwWindow(), true);

    UpdateImGuiInputState();

    renderPass = CreateRenderPass(*vulkanContext);
    framebuffers = CreateFramebuffers(renderPass, *vulkanContext);
    fontTexture = CreateFontTexture(ImGui::GetIO(), *vulkanContext);

    shaders = CreateShaderModules(vulkanContext->GetShaderManager());

    CreateGraphicsPipeline(shaders);
    CreateDescriptors();
    
    CreateWidgets(widgets, *vulkanContext);

    eventSystem->Subscribe<ES::BeforeSwapchainRecreated>(this, &UiRenderer::OnBeforeSwapchainRecreated);
    eventSystem->Subscribe<ES::SwapchainRecreated>(this, &UiRenderer::OnSwapchainRecreated);
    eventSystem->Subscribe<ES::BeforeWindowRecreated>(this, &UiRenderer::OnBeforeWindowRecreated);
    eventSystem->Subscribe<ES::WindowRecreated>(this, &UiRenderer::OnWindowRecreated);
    eventSystem->Subscribe<ES::TryReloadShaders>(this, &UiRenderer::OnTryReloadShaders);
    eventSystem->Subscribe<ES::BeforeInputModeUpdated>(this, &UiRenderer::OnBeforeInputModeUpdated);
}

UiRenderer::~UiRenderer()
{
    eventSystem->UnsubscribeAll(this);

    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eUiRenderer);

    VulkanUtils::DestroyFramebuffers(framebuffers, *vulkanContext);
}

void UiRenderer::Process(const Frame& frame, const float deltaSeconds)
{
    // Handles i/o and other different window events
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    for (const auto& widget : widgets)
    {
        widget->Process(frame, deltaSeconds);
        
        if (inputMode == InputMode::eUi || widget->IsAlwaysVisible())
        {
            widget->Build();
        }
    }

    // Creates all abstract rendering commands
    ImGui::Render();
    
    // Don't forget to update push constants
    // Ignore translate for now as there's only 1 viewport
    const ImGuiIO& io = ImGui::GetIO();
    pushConstants.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
}

void UiRenderer::Render(const Frame& frame)
{
    using namespace VulkanUtils;

    UpdateBuffers(frame.index);

    const Buffer& vertexBuffer = vertexBuffers[frame.index];
    const Buffer& indexBuffer = indexBuffers[frame.index];
    
    const VkCommandBuffer commandBuffer = frame.commandBuffer;
    
    const VkRenderPassBeginInfo beginInfo = renderPass.GetBeginInfo(framebuffers[frame.swapchainImageIndex]);

    vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetLayout(),
        0, static_cast<uint32_t>(descriptors.size()), descriptors.data(), 0, nullptr);
    
    PipelineUtils::PushConstants(commandBuffer, graphicsPipeline, "scale", pushConstants.scale);
    PipelineUtils::PushConstants(commandBuffer, graphicsPipeline, "translate", pushConstants.translate);
    
    if (const ImDrawData* drawData = ImGui::GetDrawData(); !drawData->CmdLists.empty())
    {
        const VkBuffer vkVertexBuffers[] = { vertexBuffer };
        constexpr VkDeviceSize offsets[] = { 0 };

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vkVertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        
        int32_t vertexOffset = 0;
        int32_t indexOffset = 0;
        
        const ImVec2 clipScale = ImGui::GetIO().DisplayFramebufferScale;
        VkRect2D scissorRect;

        const auto issueDraw = [&](const ImDrawCmd& command) {
            scissorRect = UiRendererDetails::GetScissor(command, clipScale);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

            vkCmdDrawIndexed(commandBuffer, command.ElemCount, 1, indexOffset, vertexOffset, 0);

            indexOffset += static_cast<int32_t>(command.ElemCount);
        };

        const auto processDrawList = [&](const ImDrawList* drawList) {
            std::ranges::for_each(drawList->CmdBuffer, issueDraw);
            vertexOffset += static_cast<int32_t>(drawList->VtxBuffer.Size);
        };

        std::ranges::for_each(drawData->CmdLists, processDrawList);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void UiRenderer::CreateGraphicsPipeline(const std::vector<ShaderModule>& shaderModules)
{
    // TODO: Multisampling for UI
    graphicsPipeline = GraphicsPipelineBuilder(*vulkanContext)
        .SetShaderModules(shaderModules)
        .SetVertexData(UiRendererDetails::GetVertexBindings(), UiRendererDetails::GetVertexAttributes())
        .SetInputTopology(InputTopology::eTriangleList)
        .SetPolygonMode(PolygonMode::eFill)
        .SetCullMode(CullMode::eNone, false)
        .SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
        .EnableBlending()
        .SetDepthState(false, false, VK_COMPARE_OP_NEVER)
        .SetRenderPass(renderPass)
        .Build();
}

void UiRenderer::CreateDescriptors()
{
    Assert(descriptors.empty());
    
    descriptors = vulkanContext->GetDescriptorSetsManager()
        .GetReflectiveDescriptorSetBuilder(graphicsPipeline, DescriptorScope::eUiRenderer)
        .Bind("fontSampler", fontTexture)
        .Build();
}

void UiRenderer::UpdateBuffers(const uint32_t frameIndex)
{
    ImDrawData* drawData = ImGui::GetDrawData();

    const VkDeviceSize vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
    const VkDeviceSize indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

    if (vertexBufferSize == 0 || indexBufferSize == 0) 
    {
        return;
    }

    Buffer& vertexBuffer = vertexBuffers[frameIndex];
    Buffer& indexBuffer = indexBuffers[frameIndex];

    if (!vertexBuffer.IsValid() || vertexBuffer.GetDescription().size < vertexBufferSize) 
    {
        BufferDescription vertexBufferDescription{ vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

        vertexBuffer = Buffer(vertexBufferDescription, false, *vulkanContext);
        std::ignore = vertexBuffer.MapMemory(); // persistent mapping
    }

    if (!indexBuffer.IsValid() || indexBuffer.GetDescription().size < indexBufferSize)
    {
        BufferDescription indexBufferDescription{ indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

        indexBuffer = Buffer(indexBufferDescription, false, *vulkanContext);
        std::ignore = indexBuffer.MapMemory(); // persistent mapping
    }

    std::span<std::byte> vertexBufferMemory = vertexBuffer.MapMemory();
    std::span<std::byte> indexBufferMemory = indexBuffer.MapMemory();

    const auto processDrawList = [&](const ImDrawList* drawList) {
        const auto vertexData = std::as_bytes(std::span(drawList->VtxBuffer));
        const auto indexData = std::as_bytes(std::span(drawList->IdxBuffer));
        
        std::ranges::copy(vertexData, vertexBufferMemory.begin());
        std::ranges::copy(indexData, indexBufferMemory.begin());
        
        vertexBufferMemory = vertexBufferMemory.subspan(vertexData.size());
        indexBufferMemory = indexBufferMemory.subspan(indexData.size());
    };

    std::ranges::for_each(drawData->CmdLists, processDrawList);
}

void UiRenderer::UpdateImGuiInputState() const
{
    ImGuiIO& io = ImGui::GetIO();
    
    if (inputMode == InputMode::eUi)
    {
        io.ConfigFlags &= ~(ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoKeyboard);
    }
    else
    {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoKeyboard;
    }
}

void UiRenderer::OnBeforeSwapchainRecreated()
{
    VulkanUtils::DestroyFramebuffers(framebuffers, *vulkanContext);
}

void UiRenderer::OnSwapchainRecreated()
{
    framebuffers = UiRendererDetails::CreateFramebuffers(renderPass, *vulkanContext);
}

void UiRenderer::OnBeforeWindowRecreated()
{
    ImGui_ImplGlfw_Shutdown();
}

void UiRenderer::OnWindowRecreated(const ES::WindowRecreated& event)
{
    ImGui_ImplGlfw_InitForOther(event.window->GetGlfwWindow(), true);
}

void UiRenderer::OnTryReloadShaders()
{
    if (std::vector<ShaderModule> reloadedShaders = UiRendererDetails::CreateShaderModules(vulkanContext->GetShaderManager());
        std::ranges::all_of(reloadedShaders, &ShaderModule::IsValid))
    {
        vulkanContext->GetDevice().WaitIdle();
        vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eUiRenderer);
        descriptors.clear();
        shaders = std::move(reloadedShaders);
        
        CreateGraphicsPipeline(shaders);
        CreateDescriptors();
    }
}

void UiRenderer::OnBeforeInputModeUpdated(const ES::BeforeInputModeUpdated& event)
{
    inputMode = event.newInputMode;
    UpdateImGuiInputState();
}

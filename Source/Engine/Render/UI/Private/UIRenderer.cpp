#include "Engine/Render/UI/UIRenderer.hpp"

#include "Engine/EngineConfig.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

namespace UIRendererDetails
{
	static constexpr std::string_view vertexShaderPath = "~/Shaders/ui.vert";
	static constexpr std::string_view fragmentShaderPath = "~/Shaders/ui.frag";

	static Image CreateFontImage(const ImGuiIO& io, const VulkanContext& vulkanContext)
	{
		unsigned char* fontData = nullptr;
		Extent2D extent = { 0, 0 };

		io.Fonts->GetTexDataAsRGBA32(&fontData, &extent.width, &extent.height);

		const auto dataSize = static_cast<size_t>(extent.width) * extent.height * 4;
		const std::span<const unsigned char> fontDataSpan = { fontData, dataSize };

		const BufferDescription bufferDescription = { .size = dataSize,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

		const Buffer fontDataBuffer(bufferDescription, false, fontDataSpan, vulkanContext);

		const ImageDescription fontImageDescription{
			.extent = extent,
			.mipLevelsCount = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

		auto fontImage = Image(fontImageDescription, vulkanContext);
		fontImage.FillMipLevel0(fontDataBuffer);

		return fontImage;
	}

    static VkSampler CreateFontImageSampler(const VkDevice device)
    {
		VkSamplerCreateInfo samplerInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		VkSampler sampler;
		const VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
		Assert(result == VK_SUCCESS);

		return sampler;
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
        using namespace VulkanHelpers;
        
        const Swapchain& swapchain = vulkanContext.GetSwapchain();
        
        std::vector<VkFramebuffer> framebuffers;
        framebuffers.reserve(swapchain.GetImageViews().size());

        std::ranges::transform(swapchain.GetImageViews(), std::back_inserter(framebuffers), [&](const ImageView& view) {
            return CreateFrameBuffer(renderPass, swapchain.GetExtent(), { view.GetVkImageView() }, vulkanContext);
        });
        
        return framebuffers;
    }

	static std::vector<ShaderModule> GetShaderModules(const ShaderManager& shaderManager)
	{
		std::vector<ShaderModule> shaderModules;
		shaderModules.reserve(2);

		shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(vertexShaderPath), ShaderType::eVertex));
		shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(fragmentShaderPath), ShaderType::eFragment));

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

	static VkRect2D GetScissor(const ImDrawCmd& command)
	{
		VkRect2D scissor;
		scissor.offset.x = std::max(static_cast<int32_t>(command.ClipRect.x), 0);
		scissor.offset.y = std::max(static_cast<int32_t>(command.ClipRect.y), 0);
		scissor.extent.width = static_cast<uint32_t>(command.ClipRect.z - command.ClipRect.x);
		scissor.extent.height = static_cast<uint32_t>(command.ClipRect.w - command.ClipRect.y);

		return scissor;
	}
}

UIRenderer::UIRenderer(EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , eventSystem{ &aEventSystem }
{
	using namespace UIRendererDetails;

	ImGui::CreateContext();
	ImGui::StyleColorsClassic();
    UpdateDisplaySize();

	renderPass = CreateRenderPass(*vulkanContext);
    framebuffers = CreateFramebuffers(renderPass, *vulkanContext);

	fontImage = CreateFontImage(ImGui::GetIO(), *vulkanContext);
	fontImageView = ImageView(fontImage, VK_IMAGE_ASPECT_COLOR_BIT, *vulkanContext);
	fontImageSampler = CreateFontImageSampler(vulkanContext->GetDevice().GetVkDevice());

	std::tie(descriptor, layout) = vulkanContext->GetDescriptorSetsManager().GetDescriptorSetBuilder()
		.Bind(0, fontImageView, fontImageSampler, VK_SHADER_STAGE_FRAGMENT_BIT)
		.Build();

	std::vector<ShaderModule> shaderModules = GetShaderModules(vulkanContext->GetShaderManager());
	Assert(std::ranges::all_of(shaderModules, &ShaderModule::IsValid));

	CreateGraphicsPipeline(std::move(shaderModules));

    eventSystem->Subscribe<ES::BeforeSwapchainRecreated>(this, &UIRenderer::OnBeforeSwapchainRecreated);
    eventSystem->Subscribe<ES::SwapchainRecreated>(this, &UIRenderer::OnSwapchainRecreated);
	eventSystem->Subscribe<ES::MouseMoved>(this, &UIRenderer::OnMouseMoved);
	eventSystem->Subscribe<ES::MouseInput>(this, &UIRenderer::OnMouseInput);
    eventSystem->Subscribe<ES::MouseWheelScrolled>(this, &UIRenderer::OnMouseWheelScrolled);
	eventSystem->Subscribe<ES::BeforeCursorModeUpdated>(this, &UIRenderer::OnBeforeCursorModeUpdated);
}

UIRenderer::~UIRenderer()
{
    eventSystem->Unsubscribe<ES::BeforeSwapchainRecreated>(this);
    eventSystem->Unsubscribe<ES::SwapchainRecreated>(this);
	eventSystem->Unsubscribe<ES::MouseMoved>(this);
	eventSystem->Unsubscribe<ES::MouseInput>(this);
    eventSystem->Unsubscribe<ES::MouseWheelScrolled>(this);
	eventSystem->Unsubscribe<ES::BeforeCursorModeUpdated>(this);

	ImGui::DestroyContext();

	VulkanHelpers::DestroySampler(vulkanContext->GetDevice().GetVkDevice(), fontImageSampler);
    VulkanHelpers::DestroyFramebuffers(framebuffers, *vulkanContext);
}

void UIRenderer::Process(const float deltaSeconds)
{
	BuildUi();
}

void UIRenderer::Render(const VkCommandBuffer commandBuffer, const uint32_t swapchainImageIndex)
{
	using namespace VulkanHelpers;

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass.GetVkRenderPass();
	renderPassInfo.framebuffer = framebuffers[swapchainImageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = vulkanContext->GetSwapchain().GetExtent();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetVkPipeline());

	const ImGuiIO& io = ImGui::GetIO();
	const VkViewport viewport = GetViewport(io.DisplaySize.x, io.DisplaySize.y);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipelineLayout(),
		0, 1, &descriptor, 0, nullptr);

	pushConstants.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
	vkCmdPushConstants(commandBuffer, graphicsPipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0,
		sizeof(PushConstants), &pushConstants);

	if (const ImDrawData* drawData = ImGui::GetDrawData(); !drawData->CmdLists.empty())
	{
		const VkBuffer vertexBuffers[] = { vertexBuffer.GetVkBuffer() };
		constexpr VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.GetVkBuffer(), 0, VK_INDEX_TYPE_UINT16);

		VkRect2D scissorRect;
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;

		const auto issueDraw = [&](const ImDrawCmd& command) {
			scissorRect = UIRendererDetails::GetScissor(command);
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

void UIRenderer::TryReloadShaders()
{
	if (std::vector<ShaderModule> shaderModules = UIRendererDetails::GetShaderModules(vulkanContext->GetShaderManager());
		std::ranges::all_of(shaderModules, &ShaderModule::IsValid))
	{
		CreateGraphicsPipeline(std::move(shaderModules));
	}
}

void UIRenderer::CreateGraphicsPipeline(std::vector<ShaderModule>&& shaderModules)
{
	graphicsPipeline = GraphicsPipelineBuilder(*vulkanContext)
		.SetDescriptorSetLayouts({ layout.GetVkDescriptorSetLayout() })
		.AddPushConstantRange({ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants) })
		.SetShaderModules(std::move(shaderModules))
		.SetVertexData(UIRendererDetails::GetVertexBindings(), UIRendererDetails::GetVertexAttributes())
		.SetInputTopology(InputTopology::eTriangleList)
		.SetPolygonMode(PolygonMode::eFill)
		.SetCullMode(CullMode::eNone, false)
		.SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
		.EnableBlending()
		.SetDepthState(false, false, VK_COMPARE_OP_NEVER)
		.SetRenderPass(renderPass)
		.Build();
}

void UIRenderer::BuildUi()
{
	using namespace EngineConfig;

	ImGui::NewFrame();

	ImGui::Begin(engineName);

	const VkPhysicalDeviceProperties& deviceProperties = vulkanContext->GetDevice().GetPhysicalDeviceProperties();

	ImGui::TextUnformatted(deviceProperties.deviceName);
	ImGui::Text("Vulkan API %i.%i.%i", VK_API_VERSION_MAJOR(deviceProperties.apiVersion),
		VK_API_VERSION_MINOR(deviceProperties.apiVersion), VK_API_VERSION_PATCH(deviceProperties.apiVersion));

	ImGui::End();

	// ImGui::ShowDemoWindow();

	ImGui::Render();	
	UpdateBuffers();
}

void UIRenderer::UpdateBuffers()
{
	ImDrawData* drawData = ImGui::GetDrawData();

	const VkDeviceSize vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
	const VkDeviceSize indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

	if (vertexBufferSize == 0 || indexBufferSize == 0) 
	{
		return;
	}

	if (!vertexBuffer.IsValid() || vertexBuffer.GetDescription().size != vertexBufferSize) 
	{
		BufferDescription vertexBufferDescription{ vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT };

		vertexBuffer = Buffer(vertexBufferDescription, false, *vulkanContext);
		std::ignore = vertexBuffer.MapMemory(true); // permanent mapping
	}

	if (!indexBuffer.IsValid() || indexBuffer.GetDescription().size != indexBufferSize)
	{
		BufferDescription indexBufferDescription{ indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT };

		indexBuffer = Buffer(indexBufferDescription, false, *vulkanContext);
		std::ignore = indexBuffer.MapMemory(true); // permanent mapping
	}

	std::span<std::byte> vertexBufferMemory = vertexBuffer.MapMemory();
	std::span<std::byte> indexBufferMemory = indexBuffer.MapMemory();

	const auto processDrawList = [&](const ImDrawList* drawList) {
		const size_t vertexDataSize = drawList->VtxBuffer.Size * sizeof(ImDrawVert);
		const size_t indexDataSize = drawList->IdxBuffer.Size * sizeof(ImDrawIdx);

		memcpy(vertexBufferMemory.data(), drawList->VtxBuffer.Data, vertexDataSize);
		memcpy(indexBufferMemory.data(), drawList->IdxBuffer.Data, indexDataSize);

		vertexBufferMemory = vertexBufferMemory.subspan(vertexDataSize);
		indexBufferMemory = indexBufferMemory.subspan(indexDataSize);
	};

	std::ranges::for_each(drawData->CmdLists, processDrawList);

	vertexBuffer.Flush();
	indexBuffer.Flush();
}

void UIRenderer::UpdateDisplaySize() const
{
    const auto [width, height] = vulkanContext->GetSwapchain().GetExtent();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
}

void UIRenderer::OnBeforeSwapchainRecreated(const ES::BeforeSwapchainRecreated& event)
{
    VulkanHelpers::DestroyFramebuffers(framebuffers, *vulkanContext);
}

void UIRenderer::OnSwapchainRecreated(const ES::SwapchainRecreated& event)
{
    UpdateDisplaySize();
    framebuffers = UIRendererDetails::CreateFramebuffers(renderPass, *vulkanContext);
}

void UIRenderer::OnMouseMoved(const ES::MouseMoved& event)
{
	if (cursorMode == CursorMode::eDisabled)
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(event.newPosition.x, event.newPosition.y);
}

void UIRenderer::OnMouseInput(const ES::MouseInput& event)
{
	if (cursorMode == CursorMode::eDisabled)
	{
		return;
	}

    if (const int buttonIndex = static_cast<int>(event.button); buttonIndex < ImGuiMouseButton_COUNT)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[buttonIndex] = event.action == MouseButtonAction::ePress;
	}
}

void UIRenderer::OnMouseWheelScrolled(const ES::MouseWheelScrolled& event)
{
    if (cursorMode == CursorMode::eDisabled)
    {
        return;
    }
    
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel = event.offset.y;
    io.MouseWheelH = event.offset.x;
}

void UIRenderer::OnBeforeCursorModeUpdated(const ES::BeforeCursorModeUpdated& event)
{
	cursorMode = event.newCursorMode;
}

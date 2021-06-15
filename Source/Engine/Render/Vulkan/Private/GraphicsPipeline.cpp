#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"

namespace GraphicsPipelineDetails
{
	// TODO: Add FileSystem and remove absolute paths
	constexpr const char* vertexShaderPath = "C:/Users/eqxba/Projects/WaterLily/Source/Shaders/vert.spv";
	constexpr const char* fragmentShaderPath = "C:/Users/eqxba/Projects/WaterLily/Source/Shaders/frag.spv";

	static std::vector<ShaderModule> GetShaderModules()
	{
		std::vector<ShaderModule> shaderModules;
		shaderModules.reserve(2);
		
		shaderModules.emplace_back(
			VulkanContext::shaderManager->CreateShaderModule(vertexShaderPath, ShaderType::eVertex));
		shaderModules.emplace_back(
			VulkanContext::shaderManager->CreateShaderModule(fragmentShaderPath, ShaderType::eFragment));
		
		return shaderModules;
	}
	
	static std::vector<VkPipelineShaderStageCreateInfo> GetShaderStageCreateInfos(
		const std::vector<ShaderModule>& shaderModules)
	{
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		shaderStages.reserve(shaderModules.size());

		for (const auto& shaderModule : shaderModules)
		{
			shaderStages.emplace_back(shaderModule.GetVkPipelineShaderStageCreateInfo());
		}
		
		return shaderStages;
	}

	static VkPipelineVertexInputStateCreateInfo GetVertexInputStateCreateInfo()
	{
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; 
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; 

		return vertexInputInfo;
	}

	static VkPipelineInputAssemblyStateCreateInfo GetInputAssemblyStateCreateInfo()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		return inputAssembly;
	}

	static std::vector<VkViewport> GetViewports(const VkExtent2D extent)
	{
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(extent.width);
		viewport.height = static_cast<float>(extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		return { viewport };
	}

	static std::vector<VkRect2D> GetScissors(const VkExtent2D extent)
	{
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = extent;

		return { scissor };
	}

	static VkPipelineViewportStateCreateInfo GetPipelineViewportStateCreateInfo(
		const std::vector<VkViewport>& viewports, const std::vector<VkRect2D>& scissors)
	{
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = static_cast<uint32_t>(viewports.size());
		viewportState.pViewports = viewports.data();
		viewportState.scissorCount = static_cast<uint32_t>(scissors.size());
		viewportState.pScissors = scissors.data();

		return viewportState;
	}

	static VkPipelineRasterizationStateCreateInfo GetRasterizationStateCreateInfo()
	{
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; 
		rasterizer.depthBiasClamp = 0.0f; 
		rasterizer.depthBiasSlopeFactor = 0.0f; 

		return rasterizer;
	}

	static VkPipelineMultisampleStateCreateInfo GetMultisampleStateCreateInfo()
	{
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; 
		multisampling.pSampleMask = nullptr; 
		multisampling.alphaToCoverageEnable = VK_FALSE; 
		multisampling.alphaToOneEnable = VK_FALSE; 

		return multisampling;
	}

	static VkPipelineColorBlendAttachmentState GetPipelineColorBlendAttachmentState()
	{
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		return colorBlendAttachment;
	}

	static VkPipelineColorBlendStateCreateInfo GetPipelineColorBlendStateCreateInfo(
		const VkPipelineColorBlendAttachmentState& colorBlendAttachment)
	{
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		return colorBlending;
	}

	static VkPipelineLayout CreatePipelineLayout()
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; 
		pipelineLayoutInfo.pSetLayouts = nullptr; 
		pipelineLayoutInfo.pushConstantRangeCount = 0; 
		pipelineLayoutInfo.pPushConstantRanges = nullptr; 

		VkPipelineLayout pipelineLayout;
		const VkResult result =
			vkCreatePipelineLayout(VulkanContext::device->device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
		Assert(result == VK_SUCCESS);

		return pipelineLayout;
	}
}

GraphicsPipeline::GraphicsPipeline(const RenderPass& renderPass)
{
	using namespace GraphicsPipelineDetails;

	// TODO: Extremely bad, need to presave this on first load because we recreate pipeline on resize
	std::vector<ShaderModule> shaderModules = GetShaderModules();	
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = GetShaderStageCreateInfos(shaderModules);
	
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = GetVertexInputStateCreateInfo();
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = GetInputAssemblyStateCreateInfo();

	// TODO: Can use dynamic viewport and scissors states and avoid recreation of the pipeline on resize (related to the
	// upper todo.
	std::vector<VkViewport> viewports = GetViewports(VulkanContext::swapchain->extent);
	std::vector<VkRect2D> scissors = GetScissors(VulkanContext::swapchain->extent);	
	VkPipelineViewportStateCreateInfo viewportState = GetPipelineViewportStateCreateInfo(viewports, scissors);

	VkPipelineRasterizationStateCreateInfo rasterizer = GetRasterizationStateCreateInfo();
	VkPipelineMultisampleStateCreateInfo multisampling = GetMultisampleStateCreateInfo();
	
	VkPipelineColorBlendAttachmentState colorBlendAttachment = GetPipelineColorBlendAttachmentState();
	VkPipelineColorBlendStateCreateInfo colorBlending = GetPipelineColorBlendStateCreateInfo(colorBlendAttachment);

	pipelineLayout = CreatePipelineLayout();

	// TODO: Move to separate function	
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass.renderPass;
	pipelineInfo.subpass = 0;
	// Can be used to create pipeline from similar one (which is faster than entirely new one)
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1; 

	// TODO: Use cache here
	const VkResult result = 
		vkCreateGraphicsPipelines(VulkanContext::device->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
	Assert(result == VK_SUCCESS);
}

GraphicsPipeline::~GraphicsPipeline()
{
	vkDestroyPipeline(VulkanContext::device->device, pipeline, nullptr);
	vkDestroyPipelineLayout(VulkanContext::device->device, pipelineLayout, nullptr);
}
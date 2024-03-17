#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Scene/Scene.hpp"

namespace GraphicsPipelineDetails
{
	// TODO: Add FileSystem and remove absolute paths
	constexpr const char* vertexShaderPath = "E:/Projects/WaterLily/Source/Shaders/vert.spv";
	constexpr const char* fragmentShaderPath = "E:/Projects/WaterLily/Source/Shaders/frag.spv";

	static std::vector<ShaderModule> GetShaderModules(const ShaderManager& shaderManager)
	{
		std::vector<ShaderModule> shaderModules;
		shaderModules.reserve(2);
		
		shaderModules.emplace_back(shaderManager.CreateShaderModule(vertexShaderPath, ShaderType::eVertex));
		shaderModules.emplace_back(shaderManager.CreateShaderModule(fragmentShaderPath, ShaderType::eFragment));
		
		return shaderModules;
	}
	
	static std::vector<VkPipelineShaderStageCreateInfo> GetShaderStageCreateInfos(
		const std::vector<ShaderModule>& shaderModules)
	{
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		shaderStages.reserve(shaderModules.size());

		std::ranges::transform(shaderModules, std::back_inserter(shaderStages), [](const ShaderModule& shaderModule) {
			return shaderModule.GetVkPipelineShaderStageCreateInfo();
		});
		
		return shaderStages;
	}

	static VkPipelineVertexInputStateCreateInfo GetVertexInputStateCreateInfo(
		  const VkVertexInputBindingDescription& bindingDescription
		, const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
	{
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; 
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); 

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

	VkPipelineDynamicStateCreateInfo GetPipelineDynamicStateCreateInfo(const std::vector<VkDynamicState>& dynamicStates)
	{
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		return dynamicState;
	}

	static VkPipelineViewportStateCreateInfo GetPipelineViewportStateCreateInfo(const uint32_t viewportsCount,
		const uint32_t scissorsCount)
	{
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = viewportsCount;
		viewportState.scissorCount = scissorsCount;

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

	static VkPipelineLayout CreatePipelineLayout(const VulkanContext& vulkanContext)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; 
		pipelineLayoutInfo.pSetLayouts = nullptr; 
		pipelineLayoutInfo.pushConstantRangeCount = 0; 
		pipelineLayoutInfo.pPushConstantRanges = nullptr; 

		VkPipelineLayout pipelineLayout;
		const VkResult result =
			vkCreatePipelineLayout(vulkanContext.GetDevice().GetVkDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout);
		Assert(result == VK_SUCCESS);

		return pipelineLayout;
	}
}

GraphicsPipeline::GraphicsPipeline(const RenderPass& renderPass, const VulkanContext& aVulkanContext)
	: vulkanContext{aVulkanContext}
{
	using namespace GraphicsPipelineDetails;

	std::vector<ShaderModule> shaderModules = GetShaderModules(vulkanContext.GetShaderManager());	
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = GetShaderStageCreateInfos(shaderModules);

	VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
    const std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = GetVertexInputStateCreateInfo(bindingDescription, 
		attributeDescriptions);
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = GetInputAssemblyStateCreateInfo();

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState = GetPipelineDynamicStateCreateInfo(dynamicStates);
	VkPipelineViewportStateCreateInfo viewportState = GetPipelineViewportStateCreateInfo(1, 1);

	VkPipelineRasterizationStateCreateInfo rasterizer = GetRasterizationStateCreateInfo();
	VkPipelineMultisampleStateCreateInfo multisampling = GetMultisampleStateCreateInfo();
	
	VkPipelineColorBlendAttachmentState colorBlendAttachment = GetPipelineColorBlendAttachmentState();
	VkPipelineColorBlendStateCreateInfo colorBlending = GetPipelineColorBlendStateCreateInfo(colorBlendAttachment);

	pipelineLayout = CreatePipelineLayout(vulkanContext);

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
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass.GetVkRenderPass();
	pipelineInfo.subpass = 0;
	// Can be used to create pipeline from similar one (which is faster than entirely new one)
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1; 

	// TODO: Use cache here
	const VkResult result = 
		vkCreateGraphicsPipelines(vulkanContext.GetDevice().GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
	Assert(result == VK_SUCCESS);
}

GraphicsPipeline::~GraphicsPipeline()
{
	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();

	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}
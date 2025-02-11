#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Scene/Scene.hpp"

namespace GraphicsPipelineDetails
{
	// TODO: Add FileSystem and remove absolute paths
    constexpr const char* vertexShaderPathWin = "E:/Projects/WaterLily/Source/Shaders/vert.spv";
    constexpr const char* fragmentShaderPathWin = "E:/Projects/WaterLily/Source/Shaders/frag.spv";

	constexpr const char* vertexShaderPathMac = "/Users/barboss/Projects/WaterLily/Source/Shaders/vert.spv";
	constexpr const char* fragmentShaderPathMac = "/Users/barboss/Projects/WaterLily/Source/Shaders/frag.spv";

    constexpr const char* vertexShaderPath = platformWin ? vertexShaderPathWin : vertexShaderPathMac;
    constexpr const char* fragmentShaderPath = platformWin ? fragmentShaderPathWin : fragmentShaderPathMac;

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

	// TODO: parse from SPIR-V reflection, create in manager and cache by description
	static std::vector<VkDescriptorSetLayout> CreateDescriptorSetLayouts(VkDevice device)
	{
		std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

		VkDescriptorSetLayoutBinding& uboLayoutBinding = bindings[0];
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding& samplerLayoutBinding = bindings[1];
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;		
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding& transformsSSBOLayoutBinding = bindings[2];
		transformsSSBOLayoutBinding.binding = 2;
		transformsSSBOLayoutBinding.descriptorCount = 1;
		transformsSSBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		transformsSSBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		transformsSSBOLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		VkDescriptorSetLayout descriptorSetLayout;

		const VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
		Assert(result == VK_SUCCESS);

		return { descriptorSetLayout };
	}

	// TODO: parse from SPIR-V reflection
	static std::vector<VkPushConstantRange> GetPushConstantRanges()
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstants);
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		return { pushConstantRange };
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
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; 
		rasterizer.depthBiasClamp = 0.0f; 
		rasterizer.depthBiasSlopeFactor = 0.0f; 

		return rasterizer;
	}

	static VkPipelineMultisampleStateCreateInfo GetMultisampleStateCreateInfo(const VulkanContext& vulkanContext)
	{
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = vulkanContext.GetDevice().GetMaxSampleCount();
		multisampling.sampleShadingEnable = VK_FALSE;		
		multisampling.minSampleShading = 1.0f; 
		multisampling.pSampleMask = nullptr; 
		multisampling.alphaToCoverageEnable = VK_FALSE; 
		multisampling.alphaToOneEnable = VK_FALSE; 

		return multisampling;
	}

	static VkPipelineDepthStencilStateCreateInfo GetPipelineDepthStencilStateCreateInfo()
	{
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {};
		depthStencil.back = {};

		return depthStencil;
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

	static VkPipelineLayout CreatePipelineLayout(VkDevice device, 
		const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, 
		const std::vector<VkPushConstantRange>& pushConstantRanges)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

		VkPipelineLayout pipelineLayout;

		const VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
		Assert(result == VK_SUCCESS);

		return pipelineLayout;
	}
}

GraphicsPipeline::GraphicsPipeline(const RenderPass& renderPass, const VulkanContext& aVulkanContext)
	: vulkanContext{aVulkanContext}
{
	using namespace GraphicsPipelineDetails;

	VkDevice device = vulkanContext.GetDevice().GetVkDevice();

	std::vector<ShaderModule> shaderModules = GetShaderModules(vulkanContext.GetShaderManager());	
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = GetShaderStageCreateInfos(shaderModules);

	descriptorSetLayouts = CreateDescriptorSetLayouts(device);

	const std::vector<VkPushConstantRange> pushConstantRanges = GetPushConstantRanges();

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
	VkPipelineMultisampleStateCreateInfo multisampling = GetMultisampleStateCreateInfo(vulkanContext);
	
	VkPipelineDepthStencilStateCreateInfo depthStencil = GetPipelineDepthStencilStateCreateInfo();

	VkPipelineColorBlendAttachmentState colorBlendAttachment = GetPipelineColorBlendAttachmentState();
	VkPipelineColorBlendStateCreateInfo colorBlending = GetPipelineColorBlendStateCreateInfo(colorBlendAttachment);

	pipelineLayout = CreatePipelineLayout(device, descriptorSetLayouts, pushConstantRanges);

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass.GetVkRenderPass();
	pipelineInfo.subpass = 0;
	// Can be used to create pipeline from similar one (which is faster than entirely new one)
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1; 

	// TODO: Use cache here
	const VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
	Assert(result == VK_SUCCESS);
}

GraphicsPipeline::~GraphicsPipeline()
{
	const VkDevice device = vulkanContext.GetDevice().GetVkDevice();

	std::ranges::for_each(descriptorSetLayouts, [=](VkDescriptorSetLayout descriptorSetLayout) {
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	});

	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

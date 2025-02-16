#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/FileSystem/FilePath.hpp"

namespace GraphicsPipelineDetails
{
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

    static VkPipelineInputAssemblyStateCreateInfo GetTriangleListInputAssembly()
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

    static VkPipelineViewportStateCreateInfo GetViewportState(const uint32_t viewportsCount,
        const uint32_t scissorsCount)
    {
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = viewportsCount;
        viewportState.scissorCount = scissorsCount;

        return viewportState;
    }

    static VkPipelineRasterizationStateCreateInfo GetDefaultRasterizer()
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

    static VkPipelineMultisampleStateCreateInfo GetMultisamplingNone()
    {
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.sampleShadingEnable = VK_FALSE;        
        multisampling.minSampleShading = 1.0f; 
        multisampling.pSampleMask = nullptr; 
        multisampling.alphaToCoverageEnable = VK_FALSE; 
        multisampling.alphaToOneEnable = VK_FALSE; 

        return multisampling;
    }

    static VkPipelineDepthStencilStateCreateInfo GetDepthStencilNone()
    {
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        return depthStencil;
    }

    static VkPipelineColorBlendAttachmentState GetDefaultAttachmentNoBlend()
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

    static VkPipelineColorBlendStateCreateInfo GetDefaultColorBlending(
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
}

GraphicsPipeline::GraphicsPipeline(const VkPipelineLayout aPipelineLayout, const VkPipeline aPipeline, 
    const VulkanContext& aVulkanContext)
    : vulkanContext{&aVulkanContext}
    , pipelineLayout{aPipelineLayout}
    , pipeline{aPipeline}
{

}

GraphicsPipeline::~GraphicsPipeline()
{
    // TODO: Deletion queue? Don't care now
    if (pipeline != VK_NULL_HANDLE)
    {
        Assert(pipelineLayout != VK_NULL_HANDLE);

        const VkDevice device = vulkanContext->GetDevice().GetVkDevice();

        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    }
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept
    : vulkanContext{other.vulkanContext}
    , pipeline{other.pipeline}
    , pipelineLayout{other.pipelineLayout}
{
    other.vulkanContext = nullptr;
    other.pipeline = VK_NULL_HANDLE;
    other.pipelineLayout = VK_NULL_HANDLE;
}

GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& other) noexcept
{
    if (this != &other)
    {
        vulkanContext = other.vulkanContext;
        pipeline = other.pipeline;
        pipelineLayout = other.pipelineLayout;

        other.vulkanContext = nullptr;
        other.pipeline = VK_NULL_HANDLE;
        other.pipelineLayout = VK_NULL_HANDLE;
    }

    return *this;
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(const VulkanContext& aVulkanContext)
    : vulkanContext{&aVulkanContext}
{
    using namespace GraphicsPipelineDetails;
    
    inputAssembly = GetTriangleListInputAssembly();
    viewportState = GetViewportState(1, 1);
    rasterizer = GetDefaultRasterizer();
    multisamplingState = GetMultisamplingNone();
    depthStencil = GetDepthStencilNone();
    colorBlendAttachment = GetDefaultAttachmentNoBlend();
    colorBlending = GetDefaultColorBlending(colorBlendAttachment);
    dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
}

GraphicsPipeline GraphicsPipelineBuilder::Build() const
{
    using namespace GraphicsPipelineDetails;
    using namespace VulkanHelpers;

    Assert(!shaderModules.empty());
    Assert(renderPass != nullptr);

    const std::vector<VkPushConstantRange> pushConstantRanges = GetPushConstantRanges();

    VkPipelineLayout pipelineLayout = CreatePipelineLayout(descriptorSetLayouts, pushConstantRanges,
        vulkanContext->GetDevice().GetVkDevice());

    const VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
    const std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::GetAttributeDescriptions();
    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = GetVertexInputStateCreateInfo(bindingDescription,
        attributeDescriptions);

    const std::vector<VkPipelineShaderStageCreateInfo> shaderStages = GetShaderStageCreateInfos(shaderModules);

    const VkPipelineDynamicStateCreateInfo dynamicState = GetPipelineDynamicStateCreateInfo(dynamicStates);

    VkGraphicsPipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisamplingState;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.renderPass = renderPass->GetVkRenderPass();
    pipelineInfo.subpass = subpass;

    // Can be used to create pipeline from similar one (which is faster than entirely new one)
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline pipeline;
    const VkResult result = vkCreateGraphicsPipelines(vulkanContext->GetDevice().GetVkDevice(),
        VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
    Assert(result == VK_SUCCESS);

    return { pipelineLayout, pipeline, *vulkanContext };
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDescriptorSetLayouts(
    std::vector<VkDescriptorSetLayout> aDescriptorSetLayouts)
{
    descriptorSetLayouts = std::move(aDescriptorSetLayouts);

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetShaderModules(std::vector<ShaderModule>&& aShaderModules)
{
    shaderModules = std::move(aShaderModules);

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetInputTopology(const InputTopology aTopology)
{
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;

    switch (aTopology)
    {
    case InputTopology::eTriangleList:
        topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    }

    Assert(topology != VK_PRIMITIVE_TOPOLOGY_MAX_ENUM);
    inputAssembly.topology = topology;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetPolygonMode(PolygonMode aPolygonMode)
{
    VkPolygonMode polygonMode = VK_POLYGON_MODE_MAX_ENUM;

    switch (aPolygonMode)
    {
    case PolygonMode::eFill:
        polygonMode = VK_POLYGON_MODE_FILL;
        break;
    }

    Assert(polygonMode != VK_POLYGON_MODE_MAX_ENUM);
    rasterizer.polygonMode = polygonMode;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetCullMode(CullMode aCullMode, bool clockwise /* = true */)
{
    VkCullModeFlags cullMode = VK_CULL_MODE_FLAG_BITS_MAX_ENUM;

    switch (aCullMode)
    {
    case CullMode::eBack:
        cullMode = VK_CULL_MODE_BACK_BIT;
        break;
    }

    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetMultisampling(const VkSampleCountFlagBits sampleCount)
{
    multisamplingState.rasterizationSamples = sampleCount;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::EnableDepthTest()
{
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetRenderPass(const RenderPass& aRenderPass, 
    const uint32_t aSubpass /* = 0 */)
{
    renderPass = &aRenderPass;
    subpass = aSubpass;

    return *this;
}
#pragma once

#include <volk.h>

#include "Engine/Render/Resources/Shaders/ShaderModule.hpp"

class VulkanContext;
class RenderPass;

enum class InputTopology
{
    eTriangleList,
};

enum class PolygonMode
{
    eFill,
};

enum class CullMode
{
    eBack,
    eNone,
};

class GraphicsPipeline
{
public:
    GraphicsPipeline() = default;
    GraphicsPipeline(VkPipelineLayout pipelineLayout, VkPipeline pipeline, const VulkanContext& vulkanContext);
	~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    GraphicsPipeline(GraphicsPipeline&& other) noexcept;
    GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;

    VkPipeline GetVkPipeline() const
	{
	    return pipeline;
	}

    VkPipelineLayout GetPipelineLayout() const
	{
	    return pipelineLayout;
	}

    bool IsValid() const
    {
        return pipelineLayout != VK_NULL_HANDLE && pipeline != VK_NULL_HANDLE;
    }

private:
    const VulkanContext* vulkanContext = nullptr;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

class GraphicsPipelineBuilder
{
public:
    using VertexBindings = std::vector<VkVertexInputBindingDescription>;
    using VertexAttributes = std::vector<VkVertexInputAttributeDescription>;

	// TODO: Add pipeline cache and manager
    GraphicsPipelineBuilder(const VulkanContext& vulkanContext);

    GraphicsPipeline Build() const;

	// TODO: Get set layouts and push constants from reflection
    GraphicsPipelineBuilder& SetDescriptorSetLayouts(std::vector<VkDescriptorSetLayout> descriptorSetLayouts); // Temp!
    GraphicsPipelineBuilder& AddPushConstantRange(VkPushConstantRange pushConstantRange); // Temp!

    GraphicsPipelineBuilder& SetShaderModules(std::vector<ShaderModule>&& shaderModules);
    GraphicsPipelineBuilder& SetVertexData(VertexBindings bindings, VertexAttributes attributes);
    GraphicsPipelineBuilder& SetInputTopology(InputTopology topology);
    GraphicsPipelineBuilder& SetPolygonMode(PolygonMode polygonMode);
    GraphicsPipelineBuilder& SetCullMode(CullMode cullMode, bool clockwise = true);
    GraphicsPipelineBuilder& SetMultisampling(VkSampleCountFlagBits sampleCount);
    GraphicsPipelineBuilder& EnableBlending();
    GraphicsPipelineBuilder& SetDepthState(bool depthTest, bool depthWrite, VkCompareOp compareOp);
    GraphicsPipelineBuilder& SetRenderPass(const RenderPass& renderPass, uint32_t subpass = 0);

private:
    const VulkanContext* vulkanContext = nullptr;

    // Temp! (parse from shaders)
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts; 
    std::vector<VkPushConstantRange> pushConstantRanges;

    std::vector<ShaderModule> shaderModules;
    VertexBindings vertexBindings;
    VertexAttributes vertexAttributes;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
    VkPipelineColorBlendStateCreateInfo colorBlendState;
    std::vector<VkDynamicState> dynamicStates;
    VkPipelineMultisampleStateCreateInfo multisamplingState;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    const RenderPass* renderPass = nullptr;
    uint32_t subpass = 0;
};
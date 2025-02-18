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
};

class GraphicsPipeline
{
public:
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

private:
    const VulkanContext* vulkanContext;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};

class GraphicsPipelineBuilder
{
public:
	// TODO: Add pipeline cache and manager
    GraphicsPipelineBuilder(const VulkanContext& vulkanContext);

    GraphicsPipeline Build() const;

	// TODO: Get set layouts and push constants from reflection
    GraphicsPipelineBuilder& SetDescriptorSetLayouts(std::vector<VkDescriptorSetLayout> descriptorSetLayouts); // Temp!

    GraphicsPipelineBuilder& SetShaderModules(std::vector<ShaderModule>&& shaderModules);
    GraphicsPipelineBuilder& SetInputTopology(InputTopology topology);
    GraphicsPipelineBuilder& SetPolygonMode(PolygonMode polygonMode);
    GraphicsPipelineBuilder& SetCullMode(CullMode cullMode, bool clockwise = true);
    GraphicsPipelineBuilder& SetMultisampling(VkSampleCountFlagBits sampleCount);
    GraphicsPipelineBuilder& EnableDepthTest();
    GraphicsPipelineBuilder& SetRenderPass(const RenderPass& renderPass, uint32_t subpass = 0);

private:
    const VulkanContext* vulkanContext;

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts; // Temp!

    std::vector<ShaderModule> shaderModules;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlending;
    std::vector<VkDynamicState> dynamicStates;
    VkPipelineMultisampleStateCreateInfo multisamplingState;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    const RenderPass* renderPass = nullptr;
    uint32_t subpass = 0;
};
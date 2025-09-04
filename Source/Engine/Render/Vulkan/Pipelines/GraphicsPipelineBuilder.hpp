#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"

class VulkanContext;
class RenderPass;

enum class InputTopology
{
    eLineList,
    eTriangleList,
};

enum class PolygonMode
{
    eFill,
    eWireframe
};

enum class CullMode
{
    eBack,
    eNone,
};

class GraphicsPipelineBuilder
{
public:
    using VertexBindings = std::vector<VkVertexInputBindingDescription>;
    using VertexAttributes = std::vector<VkVertexInputAttributeDescription>;

    // TODO: Add pipeline cache and manager
    GraphicsPipelineBuilder(const VulkanContext& vulkanContext);
    ~GraphicsPipelineBuilder() = default;

    GraphicsPipelineBuilder(const GraphicsPipelineBuilder&) = delete;
    GraphicsPipelineBuilder& operator=(const GraphicsPipelineBuilder&) = delete;

    GraphicsPipelineBuilder(GraphicsPipelineBuilder&& other) = delete;
    GraphicsPipelineBuilder& operator=(GraphicsPipelineBuilder&& other) = delete;

    Pipeline Build() const;

    GraphicsPipelineBuilder& SetShaderModules(const std::vector<ShaderModule>& shaderModules);
    GraphicsPipelineBuilder& SetSpecializationConstants(std::vector<SpecializationConstant> specializationConstants);
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

    const std::vector<ShaderModule>* shaderModules = nullptr;
    std::vector<SpecializationConstant> specializationConstants;
    VertexBindings vertexBindings;
    VertexAttributes vertexAttributes;
    std::optional<VkPipelineInputAssemblyStateCreateInfo> inputAssembly;
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

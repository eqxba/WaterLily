#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"

class ShaderManager
{
public:
	ShaderModule CreateShaderModule(const std::string& filePath, const ShaderType shaderType) const;
};
#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"
#include "Engine/FileSystem/FilePath.hpp"

class ShaderCompiler
{
public:
    static std::vector<uint32_t> Compile(std::string_view glslCode, ShaderType shaderType, const FilePath& includeDir);

    ShaderCompiler();
    ~ShaderCompiler();

    ShaderCompiler(const ShaderCompiler&) = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;

    ShaderCompiler(ShaderCompiler&&) = delete;
    ShaderCompiler& operator=(ShaderCompiler&&) = delete;
};

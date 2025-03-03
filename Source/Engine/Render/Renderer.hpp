#pragma once

#include "Engine/Render/Vulkan/Resources/Frame.hpp"

class Renderer
{
public:
    virtual ~Renderer() = default;

    virtual void Process(float deltaSeconds) = 0;
    
    virtual void Render(const Frame& frame) = 0;
};

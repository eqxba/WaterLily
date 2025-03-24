#pragma once

#include "Engine/Render/Vulkan/Frame.hpp"

class Renderer
{
public:
    virtual ~Renderer() = default;

    virtual void Process(const Frame& frame, float deltaSeconds) = 0;
    
    virtual void Render(const Frame& frame) = 0;
};

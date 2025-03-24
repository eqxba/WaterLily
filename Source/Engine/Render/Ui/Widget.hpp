#pragma once

#include "Engine/Render/Vulkan/Frame.hpp"

class Widget
{
public:
    virtual ~Widget() = default;
    
    virtual void Process(const Frame& frame, float deltaSeconds) {};
    virtual void Build() {};
    virtual bool IsAlwaysVisible() { return false; };
};

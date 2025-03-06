#pragma once

class Widget
{
public:
    virtual ~Widget() = default;
    
    virtual void Process(float deltaSeconds) {};
    virtual void Build() {};
    virtual bool IsAlwaysVisible() { return false; };
};

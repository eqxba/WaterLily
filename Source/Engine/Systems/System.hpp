#pragma once

class System
{
public:
    virtual ~System() = default;
    
    virtual void Process(float deltaSeconds) = 0;
};

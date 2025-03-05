#pragma once

#include <volk.h>

class Instance
{
public:
    Instance();
    ~Instance();

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(Instance&&) = delete;
    Instance& operator=(Instance&&) = delete;

    operator VkInstance() const
    {
        return instance;
    }

private:    
    VkInstance instance;

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
};

// Finish when i really need this one - destroy resources w/o RAII withing the frame (per frame)

//#pragma once
//
//#include <volk.h>
//
//#include "Engine/Render/Vulkan/Resources/Image.hpp"
//
//class VulkanContext;
//
//class DeletionQueue
//{
//public:
//    DeletionQueue(const VulkanContext& vulkanContext);
//    ~DeletionQueue();
//    
//    DeletionQueue(const DeletionQueue&) = delete;
//    DeletionQueue& operator=(const DeletionQueue&) = delete;
//
//    DeletionQueue(DeletionQueue&& other) noexcept;
//    DeletionQueue& operator=(DeletionQueue&& other) noexcept;
//    
//    void Flush();
//    
//    // Const ref? I don't need to move actually.............
//    void Add(Image image);
//    
//private:
//    const VulkanContext* vulkanContext = nullptr;
//    
//    std::vector<Image> images;
//};

//#include "Engine/Render/Vulkan/Resources/DeletionQueue.hpp"
//
//#include "Engine/Render/Vulkan/VulkanContext.hpp"
//
//DeletionQueue::DeletionQueue(const VulkanContext& aVulkanContext)
//    : vulkanContext{ &aVulkanContext }
//{
//    
//}
//
//DeletionQueue::~DeletionQueue()
//{
//    Assert(images.empty());
//}
//
//DeletionQueue::DeletionQueue(DeletionQueue&& other) noexcept
//    : vulkanContext{ other.vulkanContext }
//    , images{ std::move(other.images) }
//{
//    other.vulkanContext = nullptr;
//}
//
//DeletionQueue& DeletionQueue::operator=(DeletionQueue&& other) noexcept
//{
//    if (this != &other)
//    {
//        std::swap(vulkanContext, other.vulkanContext);
//        std::swap(images, other.images);
//    }
//    return *this;
//}
//
//void DeletionQueue::Flush()
//{
//    MemoryManager& memoryManager = vulkanContext->GetMemoryManager();
//    
//    const auto destroyImage = [&](const Image& image) {
//        memoryManager.DestroyImage(image.Get());
//    };
//    
//    std::ranges::for_each(images, destroyImage);
//    images.clear();
//}
//
//void DeletionQueue::Add(Image image)
//{
//    images.push_back(std::move(image));
//    
//}

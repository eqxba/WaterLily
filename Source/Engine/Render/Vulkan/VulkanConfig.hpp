#pragma once

#include <volk.h>

namespace VulkanConfig
{
    constexpr uint32_t apiVersion = VK_API_VERSION_1_3;

#ifdef NDEBUG
    constexpr bool useValidation = false;
#else
    constexpr bool useValidation = true;
#endif

    const std::vector<const char*> requiredDeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef PLATFORM_MAC
        "VK_KHR_portability_subset"
#endif
    };

    constexpr uint32_t maxFramesInFlight = 2;

    constexpr VkFormat depthImageFormat = VK_FORMAT_D32_SFLOAT;

    constexpr uint32_t maxSetsInPool = 1000;

    constexpr auto defaultPoolSizeRatios = std::to_array<std::pair<VkDescriptorType, float>>({
        { VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
    });
}

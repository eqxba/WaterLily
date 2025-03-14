#pragma once

#include <volk.h>

namespace VulkanConfig
{
    inline constexpr uint32_t apiVersion = VK_API_VERSION_1_3;

#ifdef NDEBUG
    inline constexpr bool useValidation = false;
#else
    inline constexpr bool useValidation = true;
#endif

    inline constexpr auto requiredDeviceExtensions = std::to_array<const char*>({
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef PLATFORM_MAC
        "VK_KHR_portability_subset"
#endif
    });

    inline constexpr uint32_t maxFramesInFlight = 2;

    inline constexpr VkFormat depthImageFormat = VK_FORMAT_D32_SFLOAT;

    inline constexpr uint32_t maxSetsInPool = 1000;

    inline constexpr auto defaultPoolSizeRatios = std::to_array<std::pair<VkDescriptorType, float>>({
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

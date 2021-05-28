#pragma once

namespace VulkanConfig
{
#ifdef NDEBUG
	constexpr bool useValidation = false;
#else
	constexpr bool useValidation = true;
#endif
}

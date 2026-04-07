#pragma once

#ifndef FUSION_VULKAN_VALIDATION
#   if _DEBUG
#       define FUSION_VULKAN_VALIDATION 1
#   else
#       define FUSION_VULKAN_VALIDATION 0
#   endif
#endif

namespace Fusion::Vulkan
{
    struct FVulkanTempSurfaceBase
    {
		VkSurfaceKHR tempSurface = VK_NULL_HANDLE;
    };

    class FVulkanPlatformBase
    {
    public:

        FVulkanPlatformBase() = delete;
		~FVulkanPlatformBase() = delete;

		

    };
    
} // namespace Fusion

#if FUSION_PLATFORM_WINDOWS
#include "PAL/Windows/VulkanWindowsPlatform.h"
#undef max
#elif FUSION_PLATFORM_MAC
#include "PAL/Mac/VulkanMacPlatform.h"
#elif FUSION_PLATFORM_LINUX

#endif

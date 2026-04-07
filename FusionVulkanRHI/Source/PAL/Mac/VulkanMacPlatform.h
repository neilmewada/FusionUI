#pragma once

#include "vulkan/vulkan_metal.h"

namespace Fusion::Vulkan
{
    struct FMacVulkanTempSurface : FVulkanTempSurfaceBase
    {
        void* nsWindow   = nullptr;  // NSWindow* (opaque to C++)
        void* metalLayer = nullptr;  // CAMetalLayer* (opaque to C++)
    };

    class FVulkanMacPlatform : public FVulkanPlatformBase
    {
    public:
        FVulkanMacPlatform() = delete;
        ~FVulkanMacPlatform() = delete;

        static FArray<const char*> GetRequiredVulkanInstanceExtensions();
        static FArray<const char*> GetRequiredInstanceLayers();
        static VkInstanceCreateFlags GetRequiredInstanceFlags();
        static bool IsValidationEnabled();

        static FMacVulkanTempSurface CreateTempSurface(VkInstance instance);
        static void DestroyTempSurface(VkInstance instance, FMacVulkanTempSurface& tempSurface);
        static VkSurfaceKHR CreateSurface(FVulkanRenderBackend* renderBackend, FWindowHandle window);
    };

    typedef FVulkanMacPlatform FVulkanPlatform;
    typedef FMacVulkanTempSurface FVulkanTempSurface;

} // namespace Fusion::Vulkan

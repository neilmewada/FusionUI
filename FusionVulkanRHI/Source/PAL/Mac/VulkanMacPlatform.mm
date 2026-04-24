// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#include "VulkanRHIPrivate.h"
#include "PAL/Mac/VulkanMacPlatform.h"

namespace Fusion::Vulkan
{
    TArray<const char*> FVulkanMacPlatform::GetRequiredVulkanInstanceExtensions()
    {
        return {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_EXT_METAL_SURFACE_EXTENSION_NAME,
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#if FUSION_VULKAN_VALIDATION
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        };
    }

    TArray<const char*> FVulkanMacPlatform::GetRequiredInstanceLayers()
    {
        return {
#if FUSION_VULKAN_VALIDATION
            "VK_LAYER_KHRONOS_validation",
#endif
        };
    }

    VkInstanceCreateFlags FVulkanMacPlatform::GetRequiredInstanceFlags()
    {
        return VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    bool FVulkanMacPlatform::IsValidationEnabled()
    {
#if FUSION_VULKAN_VALIDATION
        return true;
#else
        return false;
#endif
    }

    // ---------------------------------------------------------------------------

    static CAMetalLayer* AttachMetalLayer(NSView* view)
    {
        [view setWantsLayer:YES];
        CAMetalLayer* layer = [CAMetalLayer layer];
        if (view.window)
            layer.contentsScale = view.window.backingScaleFactor;
        [view setLayer:layer];
        return layer;
    }

    // ---------------------------------------------------------------------------

    FMacVulkanTempSurface FVulkanMacPlatform::CreateTempSurface(VkInstance instance)
    {
        FMacVulkanTempSurface surf{};

        NSWindow* win = [[NSWindow alloc]
            initWithContentRect:NSMakeRect(0, 0, 1, 1)
                      styleMask:NSWindowStyleMaskBorderless
                        backing:NSBackingStoreBuffered
                          defer:NO];

        NSView* view = win.contentView;
        CAMetalLayer* layer = [CAMetalLayer layer];
        [view setWantsLayer:YES];
        [view setLayer:layer];

        VkMetalSurfaceCreateInfoEXT surfCI{};
        surfCI.sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        surfCI.pLayer = layer;
        vkCreateMetalSurfaceEXT(instance, &surfCI, nullptr, &surf.tempSurface);

        surf.nsWindow   = (__bridge_retained void*)win;
        surf.metalLayer = (__bridge void*)layer;

        return surf;
    }

    void FVulkanMacPlatform::DestroyTempSurface(VkInstance instance, FMacVulkanTempSurface& surf)
    {
        if (surf.tempSurface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(instance, surf.tempSurface, nullptr);
            surf.tempSurface = VK_NULL_HANDLE;
        }

        if (surf.nsWindow)
        {
            NSWindow* win = (__bridge_transfer NSWindow*)surf.nsWindow;
            [win close];
            surf.nsWindow   = nullptr;
            surf.metalLayer = nullptr;
        }
    }

    VkSurfaceKHR FVulkanMacPlatform::CreateSurface(FVulkanRenderBackend* renderBackend, FWindowHandle window)
    {
        if (window.IsNull() || !renderBackend)
            return VK_NULL_HANDLE;

        // GetNativeWindowHandle returns NSWindow* cast to void*
        void* rawWindow = renderBackend->GetPlatformBackend()->GetNativeWindowHandle(window);
        if (!rawWindow)
            return VK_NULL_HANDLE;

        NSWindow* nsWindow = (__bridge NSWindow*)rawWindow;
        NSView* view = nsWindow.contentView;
        CAMetalLayer* layer = AttachMetalLayer(view);

        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkMetalSurfaceCreateInfoEXT surfCI{};
        surfCI.sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        surfCI.pLayer = layer;
        vkCreateMetalSurfaceEXT(renderBackend->GetVkInstance(), &surfCI, nullptr, &surface);

        return surface;
    }

} // namespace Fusion::Vulkan

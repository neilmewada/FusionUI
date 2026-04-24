#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "vulkan/vulkan_win32.h"

namespace Fusion::Vulkan
{
	struct FWindowsVulkanTempSurface : FVulkanTempSurfaceBase
    {
	    HWND hwnd = nullptr;
    };
    
	class FVulkanWindowsPlatform : public FVulkanPlatformBase
    {
    public:

		FVulkanWindowsPlatform() = delete;

        ~FVulkanWindowsPlatform() = delete;

        static TArray<const char*> GetRequiredVulkanInstanceExtensions()
        {
            return {
	            VK_KHR_SURFACE_EXTENSION_NAME,
	            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#if FUSION_VULKAN_VALIDATION
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
            };
        }

        static TArray<const char*> GetRequiredInstanceLayers()
        {
            return {
#if FUSION_VULKAN_VALIDATION
                "VK_LAYER_KHRONOS_validation",
#endif
            };
        }

        static VkInstanceCreateFlags GetRequiredInstanceFlags() { return 0; }

        static bool IsValidationEnabled()
        {
#if FUSION_VULKAN_VALIDATION
            return true;
#else
			return false;
#endif
        }

        static FWindowsVulkanTempSurface CreateTempSurface(VkInstance instance)
        {
            FWindowsVulkanTempSurface tempSurfaceData;

            HINSTANCE hInstance = GetModuleHandleA(nullptr);

            WNDCLASSA wc{};
            wc.lpfnWndProc   = DefWindowProcA;
            wc.hInstance     = hInstance;
            wc.lpszClassName = "FusionTempVulkanWindowClass";
            RegisterClassA(&wc);

            tempSurfaceData.hwnd = CreateWindowExA(
                0, "FusionTempVulkanWindowClass", "",
                WS_POPUP,
                0, 0, 1, 1,
                nullptr, nullptr, hInstance, nullptr
            );

            VkWin32SurfaceCreateInfoKHR surfaceInfo{};
            surfaceInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            surfaceInfo.hinstance = hInstance;
            surfaceInfo.hwnd      = tempSurfaceData.hwnd;
            vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, &tempSurfaceData.tempSurface);

            return tempSurfaceData;
        }

        static void DestroyTempSurface(VkInstance instance, FWindowsVulkanTempSurface& tempSurface)
        {
            if (tempSurface.tempSurface != VK_NULL_HANDLE)
            {
                vkDestroySurfaceKHR(instance, tempSurface.tempSurface, nullptr);
                tempSurface.tempSurface = VK_NULL_HANDLE;
            }

            if (tempSurface.hwnd != nullptr)
            {
                DestroyWindow(tempSurface.hwnd);
                tempSurface.hwnd = nullptr;
            }
        }

        static VkSurfaceKHR CreateSurface(FVulkanRenderBackend* renderBackend, FWindowHandle window)
        {
            if (window.IsNull() || !renderBackend)
                return VK_NULL_HANDLE;

            HWND hwnd = (HWND)renderBackend->GetPlatformBackend()->GetNativeWindowHandle(window);
            if (!hwnd)
                return VK_NULL_HANDLE;

            VkSurfaceKHR surface = VK_NULL_HANDLE;

            VkWin32SurfaceCreateInfoKHR surfaceInfo{};
            surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            surfaceInfo.hinstance = GetModuleHandleA(nullptr);
            surfaceInfo.hwnd = hwnd;
            vkCreateWin32SurfaceKHR(renderBackend->GetVkInstance(), &surfaceInfo, nullptr, &surface);

            return surface;
        }
    };

	typedef FVulkanWindowsPlatform FVulkanPlatform;
	typedef FWindowsVulkanTempSurface FVulkanTempSurface;

} // namespace Fusion

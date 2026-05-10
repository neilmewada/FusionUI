#pragma once

#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <X11/Xlib.h>
#include <cstring>
#include <cstdlib>
#include "vulkan/vulkan_wayland.h"
#include "vulkan/vulkan_xlib.h"

namespace Fusion::Vulkan
{

    struct FLinuxVulkanTempSurface : FVulkanTempSurfaceBase
    {
        // Wayland
        wl_display*    display    = nullptr;
        wl_compositor* compositor = nullptr;
        wl_surface*    surface    = nullptr;
        // X11
        void*     x11Display = nullptr;  // Display*
        uintptr_t x11Window  = 0;        // Window (XID)
    };

    class FVulkanLinuxPlatform : public FVulkanPlatformBase
    {
    public:

        FVulkanLinuxPlatform() = delete;
        ~FVulkanLinuxPlatform() = delete;

        static TArray<const char*> GetRequiredVulkanInstanceExtensions()
        {
            return {
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
                VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
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

        static bool IsWayland()
        {
            const char* wd = getenv("WAYLAND_DISPLAY");
            return wd && wd[0] != '\0';
        }

        static bool IsValidationEnabled()
        {
#if FUSION_VULKAN_VALIDATION
            return true;
#else
            return false;
#endif
        }

        static FLinuxVulkanTempSurface CreateTempSurface(FVulkanRenderBackend* renderBackend, VkInstance instance)
        {
            FLinuxVulkanTempSurface tempSurfaceData;

            if (IsWayland())
            {
                wl_display* display = (wl_display*)renderBackend->GetPlatformBackend()->GetNativeDisplayHandle({});
                if (!display)
                    return tempSurfaceData;

                struct FRegistryContext { wl_compositor* compositor = nullptr; };

                static const wl_registry_listener kRegistryListener = {
                    [](void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
                    {
                        auto* ctx = (FRegistryContext*)data;
                        if (strcmp(interface, wl_compositor_interface.name) == 0)
                            ctx->compositor = (wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, 1);
                    },
                    [](void*, wl_registry*, uint32_t) {}
                };

                FRegistryContext ctx{};
                wl_registry* registry = wl_display_get_registry(display);
                wl_registry_add_listener(registry, &kRegistryListener, &ctx);
                wl_display_roundtrip(display);
                wl_registry_destroy(registry);

                if (!ctx.compositor)
                    return tempSurfaceData;

                wl_surface* surface = wl_compositor_create_surface(ctx.compositor);

                VkWaylandSurfaceCreateInfoKHR surfaceInfo{};
                surfaceInfo.sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
                surfaceInfo.display = display;
                surfaceInfo.surface = surface;
                vkCreateWaylandSurfaceKHR(instance, &surfaceInfo, nullptr, &tempSurfaceData.tempSurface);

                tempSurfaceData.display    = display;
                tempSurfaceData.compositor = ctx.compositor;
                tempSurfaceData.surface    = surface;
            }
            else
            {
                // X11 path: open our own display connection for the temp surface
                Display* dpy = XOpenDisplay(nullptr);
                if (!dpy)
                    return tempSurfaceData;

                int screen = DefaultScreen(dpy);
                Window xwin = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 0, 0, 1, 1, 0, 0, 0);

                VkXlibSurfaceCreateInfoKHR surfaceInfo{};
                surfaceInfo.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
                surfaceInfo.dpy    = dpy;
                surfaceInfo.window = xwin;
                vkCreateXlibSurfaceKHR(instance, &surfaceInfo, nullptr, &tempSurfaceData.tempSurface);

                tempSurfaceData.x11Display = dpy;
                tempSurfaceData.x11Window  = (uintptr_t)xwin;
            }

            return tempSurfaceData;
        }

        static void DestroyTempSurface(VkInstance instance, FLinuxVulkanTempSurface& tempSurface)
        {
            if (tempSurface.tempSurface != VK_NULL_HANDLE)
            {
                vkDestroySurfaceKHR(instance, tempSurface.tempSurface, nullptr);
                tempSurface.tempSurface = VK_NULL_HANDLE;
            }

            if (IsWayland())
            {
                if (tempSurface.surface)
                {
                    wl_surface_destroy(tempSurface.surface);
                    tempSurface.surface = nullptr;
                }
                if (tempSurface.compositor)
                {
                    wl_compositor_destroy(tempSurface.compositor);
                    tempSurface.compositor = nullptr;
                }
                tempSurface.display = nullptr;
            }
            else
            {
                if (tempSurface.x11Display)
                {
                    if (tempSurface.x11Window)
                        XDestroyWindow((Display*)tempSurface.x11Display, (Window)tempSurface.x11Window);
                    XCloseDisplay((Display*)tempSurface.x11Display);
                    tempSurface.x11Display = nullptr;
                    tempSurface.x11Window  = 0;
                }
            }
        }

        static VkSurfaceKHR CreateSurface(FVulkanRenderBackend* renderBackend, FWindowHandle window)
        {
            if (window.IsNull() || !renderBackend)
                return VK_NULL_HANDLE;

            void* nativeWindow  = renderBackend->GetPlatformBackend()->GetNativeWindowHandle(window);
            void* nativeDisplay = renderBackend->GetPlatformBackend()->GetNativeDisplayHandle(window);
            if (!nativeWindow || !nativeDisplay)
                return VK_NULL_HANDLE;

            VkSurfaceKHR vkSurface = VK_NULL_HANDLE;

            if (IsWayland())
            {
                VkWaylandSurfaceCreateInfoKHR surfaceInfo{};
                surfaceInfo.sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
                surfaceInfo.display = (wl_display*)nativeDisplay;
                surfaceInfo.surface = (wl_surface*)nativeWindow;
                vkCreateWaylandSurfaceKHR(renderBackend->GetVkInstance(), &surfaceInfo, nullptr, &vkSurface);
            }
            else
            {
                VkXlibSurfaceCreateInfoKHR surfaceInfo{};
                surfaceInfo.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
                surfaceInfo.dpy    = (Display*)nativeDisplay;
                surfaceInfo.window = (Window)(uintptr_t)nativeWindow;
                vkCreateXlibSurfaceKHR(renderBackend->GetVkInstance(), &surfaceInfo, nullptr, &vkSurface);
            }

            return vkSurface;
        }
    };

    typedef FVulkanLinuxPlatform FVulkanPlatform;
    typedef FLinuxVulkanTempSurface FVulkanTempSurface;

} // namespace Fusion::Vulkan

#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    // - Shader -

    struct FUSIONVULKANRHI_API FGraphicsPipeline : IntrusiveBase
    {
    public:

        FGraphicsPipeline(VkDevice device);

        ~FGraphicsPipeline();

        
        VkDevice m_Device = nullptr;

        VkShaderModule m_VertexModule = VK_NULL_HANDLE;
        VkShaderModule m_FragmentModule = VK_NULL_HANDLE;

        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
    };

    struct FUSIONVULKANRHI_API FSwapChain : IntrusiveBase
    {
    public:

        FSwapChain(VkInstance instance, VkDevice device);

        ~FSwapChain();

        VkInstance m_Instance = nullptr;
        VkDevice m_Device = nullptr;

        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;

    };

    // - Render Instance -

    struct FUSIONVULKANRHI_API FRenderInstance : IntrusiveBase
    {
        // Per ApplicationInstance data goes here

    };
    
    class FUSIONVULKANRHI_API FVulkanRenderBackend : public IFRenderBackend, public IFPlatformEventSink
    {
    public:

        static constexpr u32 ImageCount = 2;

        FVulkanRenderBackend(IFPlatformBackend* platformBackend) : IFRenderBackend(platformBackend)
        {
	        
        }

        VkInstance GetVkInstance()
        {
            return m_VulkanInstance;
        }

        FGraphicsBackendType GetGraphicsBackendType() override
        {
            return FGraphicsBackendType::Vulkan;
        }

        FRenderCapabilities GetRenderCapabilities() override;

        bool IsInitialized(FInstanceHandle instance) override
        {
            return instances.KeyExists(instance);
        }

        bool InitializeInstance(FInstanceHandle instance) override;

        void ShutdownInstance(FInstanceHandle instance) override;

	private:

		// - Vulkan Lifecycle -

		void InitializeVulkan();

		void ShutdownVulkan();

        // - Window & SwapChain -

        void CreateOrUpdateSwapChain(FWindowHandle window);

        void OnWindowCreated(FWindowHandle window) override;

        void OnWindowDestroyed(FWindowHandle window) override;

        void OnWindowResized(FWindowHandle window, u32 newWidth, u32 newHeight) override;

        void OnWindowMaximized(FWindowHandle window) override;

        void OnWindowMinimized(FWindowHandle window) override;

        void OnWindowRestored(FWindowHandle window) override;

    private:

        HashMap<FInstanceHandle, IntrusivePtr<FRenderInstance>> instances;

        // - Vulkan Data -

		VkInstance m_VulkanInstance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_VkMessenger = VK_NULL_HANDLE;

		// - Physical Device -

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties{};
		VkPhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties{};

        FArray<VkQueueFamilyProperties> m_QueueFamilyProperties;

        // - Properties -

		bool m_IsUnifiedMemory = false;
        bool m_IsResizableBAR = false;

        // - Surface -

        VkSurfaceCapabilitiesKHR m_SurfaceCapabilities{};   // TestSurface properties: image size, extent, etc
        FArray<VkSurfaceFormatKHR> m_SurfaceFormats;  // TestSurface image supported formats
        FArray<VkPresentModeKHR> m_PresentationModes; // How images should be presented to the screen

        // - Device -

		VkDevice m_Device = VK_NULL_HANDLE;

        // - Queues -
        
        int m_QueueFamilyIndex = -1;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;

        // - Command Pools & Buffers -

        VkCommandPool m_CommandPool = VK_NULL_HANDLE;

        // - SwapChain -

        HashMap<FWindowHandle, IntrusivePtr<FSwapChain>> m_SwapChainsByWindowHandle;

        // - Render Pass -

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;

        // - Pipeline -

        IntrusivePtr<FGraphicsPipeline> m_MainGraphicsPipeline;

    };

} // namespace Fusion

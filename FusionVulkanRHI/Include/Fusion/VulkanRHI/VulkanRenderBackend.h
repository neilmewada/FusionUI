#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion::Vulkan
{
    class FVulkanRenderBackend;

    // - Descriptor Set -

    struct FUSIONVULKANRHI_API FDescriptorSet
    {
        FDescriptorSet(FVulkanRenderBackend* renderBackend, VkDevice device, VkDescriptorPool pool)
	        : m_RenderBackend(renderBackend), m_Device(device), m_OwningPool(pool)
		{}

        ~FDescriptorSet();

        FVulkanRenderBackend* m_RenderBackend = nullptr;
        VkDevice m_Device = VK_NULL_HANDLE;

        VkDescriptorPool m_OwningPool = VK_NULL_HANDLE;
        VkDescriptorSet m_Set = VK_NULL_HANDLE;
    };

    // - Descriptor Pool -

    struct FUSIONVULKANRHI_API FDescriptorPool
    {
        FDescriptorPool(FVulkanRenderBackend* renderBackend, VkDevice device) 
    		: m_RenderBackend(renderBackend), m_Device(device)
        {
            Grow();
        }

        ~FDescriptorPool();

        void Grow();

        FDescriptorSet* Allocate(VkDescriptorSetLayout setLayout);

        FVulkanRenderBackend* m_RenderBackend = nullptr;
        VkDevice m_Device = VK_NULL_HANDLE;
        
        FArray<VkDescriptorPool> m_Pools{};
    };

    // - Image -

    struct FUSIONVULKANRHI_API FTexture : IntrusiveBase
    {
        FTexture(FVulkanRenderBackend* renderBackend, VkDevice device) : m_RenderBackend(renderBackend), m_Device(device)
        {
	        
        }

        ~FTexture();

        FVulkanRenderBackend* m_RenderBackend = nullptr;
        VkDevice m_Device = VK_NULL_HANDLE;

        bool m_IsExternalImage = false;

        VkImage m_Image = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;

        VkFormat m_Format = VK_FORMAT_R8G8B8A8_UNORM;
    };

    // - Shader -

    struct FUSIONVULKANRHI_API FGraphicsPipeline : IntrusiveBase
    {
    public:

        FGraphicsPipeline(VkDevice device);

        ~FGraphicsPipeline();

        
        VkDevice m_Device = VK_NULL_HANDLE;

        VkShaderModule m_VertexModule = VK_NULL_HANDLE;
        VkShaderModule m_FragmentModule = VK_NULL_HANDLE;

        FArray<VkDescriptorSetLayout> m_SetLayouts{};
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
    };

    struct FUSIONVULKANRHI_API FSwapChain : IntrusiveBase
    {
    public:

        FSwapChain(FVulkanRenderBackend* renderBackend, VkDevice device);

        ~FSwapChain();

        FVulkanRenderBackend* m_RenderBackend = nullptr;
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;

        FWindowHandle m_Window = FWindowHandle::NullValue;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;

        FArray<IntrusivePtr<FTexture>> m_Images{};
        FArray<VkFramebuffer> m_FrameBuffers{};

        u32 width = 0, height = 0;

        FArray<VkSemaphore> m_ImageAcquiredSemaphores{};

        uint32_t m_CurrentImageIndex = -1;
    };

    // - Render Instance -

    struct FUSIONVULKANRHI_API FRenderInstance : IntrusiveBase
    {
        // Per ApplicationInstance data goes here

    };

    // - Render Backend -

    struct FDeferredDestruction
    {
        int m_FrameCounter = 2;

        FDelegate<void()> m_Destruction{};
    };
    
    class FUSIONVULKANRHI_API FVulkanRenderBackend : public IFRenderBackend
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

        bool IsDescriptorPoolAlive() const
        {
            return m_Pool != nullptr;
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

        void RenderTick() override;

        template<typename TFunc>
        void DeferDestruction(TFunc&& functor)
        {
            m_DeferredDestruction.Add({
                .m_FrameCounter = ImageCount,
                .m_Destruction = functor
            });
        }

	private:

        void TickDestructionQueue();

		// - Vulkan Lifecycle -

		void InitializeVulkan();

		void ShutdownVulkan();

        // - Window & SwapChain -

        void UpdateAllSwapChains();

        void CreateOrUpdateSwapChain(FWindowHandle window);

        void DestroySwapChain(FWindowHandle window);

        void OnWindowCreated(FWindowHandle window) override;

        void OnWindowDestroyed(FWindowHandle window) override;

        void OnWindowResized(FWindowHandle window, u32 newWidth, u32 newHeight) override;

        void OnWindowMaximized(FWindowHandle window) override;

        void OnWindowMinimized(FWindowHandle window) override;

        void OnWindowRestored(FWindowHandle window) override;

    private:

        FHashMap<FInstanceHandle, IntrusivePtr<FRenderInstance>> instances;

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
        uint32_t m_QueueCount = 0;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;

        // - Command Pools & Buffers -

        VkCommandPool m_CommandPool = VK_NULL_HANDLE;

        // - SwapChain -

        FHashMap<FWindowHandle, IntrusivePtr<FSwapChain>> m_SwapChainsByWindowHandle;

        // - Render Pass -

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;

        // - Pipeline -

        IntrusivePtr<FGraphicsPipeline> m_MainGraphicsPipeline;

        // - Descriptors -

        FDescriptorPool* m_Pool = nullptr;

        // - Frame Loop -

        u32 m_FrameSlot = 0;
        FArray<VkCommandBuffer, ImageCount> m_CommandBuffers;
        FArray<VkSemaphore, ImageCount> m_RenderFinishedSemaphores;
        FArray<VkFence, ImageCount> m_RenderFinishedFences;

        FArray<FDeferredDestruction> m_DeferredDestruction;
    };

} // namespace Fusion

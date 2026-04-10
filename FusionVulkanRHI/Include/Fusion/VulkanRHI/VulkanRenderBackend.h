#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion::Vulkan
{
    class FVulkanRenderBackend;
    class FTexture;

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

        void Reset();

        VkDescriptorSet Allocate(VkDescriptorSetLayout setLayout);

        FVulkanRenderBackend* m_RenderBackend = nullptr;
        VkDevice m_Device = VK_NULL_HANDLE;
        
        int m_CurPoolIndex = 0;
        FArray<VkDescriptorPool> m_Pools{};
    };

    // - Shader -

    struct FUSIONVULKANRHI_API FGraphicsPipeline : FIntrusiveBase
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

    struct FUSIONVULKANRHI_API FSwapChain : FIntrusiveBase
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

        FArray<IPtr<FTexture>> m_Images{};
        FArray<VkFramebuffer> m_FrameBuffers{};

        u32 m_Width = 0, m_Height = 0;

        FArray<VkSemaphore> m_ImageAcquiredSemaphores{};

        uint32_t m_CurrentImageIndex = -1;
    };

    struct FRenderTarget : FIntrusiveBase
    {
        ERenderTargetType m_Type = ERenderTargetType::Window;

        FWindowHandle m_Window{};

        IPtr<FRenderSnapshot> m_Snapshot = nullptr;
    };

    // - Render Instance -

    struct FUSIONVULKANRHI_API FRenderInstance : FIntrusiveBase
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

        static constexpr u32 kImageCount = 2;
        static constexpr VkDeviceSize kVertexBufferOffsetAlignment = 4;
        static constexpr VkDeviceSize kIndexBufferOffsetAlignment = 4;

        static constexpr VkDeviceSize kBufferInitialSize = 16_MB;
        static constexpr VkDeviceSize kBufferGrowSize = 16_MB;

        static constexpr VkDeviceSize kStagingBufferInitialSize = 16_MB;
        static constexpr VkDeviceSize kStagingBufferGrowSize = 16_MB;

        static constexpr SizeT kUploadArenaGrowSize = 16_MB;

        FVulkanRenderBackend(IFPlatformBackend* platformBackend) : IFRenderBackend(platformBackend)
        {
	        
        }

        VkInstance GetVkInstance()
        {
            return m_VulkanInstance;
        }

        VkDevice GetVkDevice()
        {
            return m_Device;
        }

        VmaAllocator GetVmaAllocator()
        {
            return m_Allocator;
        }

        bool IsUnifiedMemory() const
        {
            return m_IsUnifiedMemory;
        }

        bool IsResizableBAR() const
        {
            return m_IsResizableBAR;
        }

        VkQueue GetGraphicsQueue() const
        {
            return m_GraphicsQueue;
        }

        const VkPhysicalDeviceFeatures& GetFeatures() const
        {
            return m_PhysicalDeviceFeatures;
        }

        EGraphicsBackendType GetGraphicsBackendType() override
        {
            return EGraphicsBackendType::Vulkan;
        }

        FRenderBackendCapabilities GetRenderCapabilities() override;

        bool IsInitialized(FInstanceHandle instance) override
        {
            return instances.KeyExists(instance);
        }

        bool InitializeInstance(FInstanceHandle instance) override;

        void ShutdownInstance(FInstanceHandle instance) override;

        void RenderTick() override;

        void SubmitSnapshot(FRenderTargetHandle renderTarget, IntrusivePtr<FRenderSnapshot> snapshot) override;

        template<typename TFunc>
        void DeferDestruction(TFunc&& functor)
        {
            m_DeferredDestruction.Add({
                .m_FrameCounter = kImageCount,
                .m_Destruction = functor
            });
        }

        // - Atlas -

        FAtlasHandle CreateLayeredAtlas(bool grayscale, u32 resolution, u32 maxLayers) override;

        void UploadAtlasRegionAsync(FAtlasHandle atlas, u32 layer,
            FVec2i pos, FVec2i size,
            const u8* pixels, int pitch) override;

        void DestroyAtlas(FAtlasHandle atlas) override;

        // - Render Target -

        FRenderTargetHandle AcquireWindowRenderTarget(FWindowHandle window) override;

        void ReleaseRenderTarget(FRenderTargetHandle renderTarget) override;

	private:

        void TickDestructionQueue();

		// - Vulkan Lifecycle -

		void InitializeVulkan();

		void ShutdownVulkan();

        void TransitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout curLayout, VkImageLayout toLayout, VkImageSubresourceRange subresourceRange);

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

        using FArena = FStableGrowthArray<u8, kUploadArenaGrowSize>;
        using FBufferImageCopyArray = FStableGrowthArray<VkBufferImageCopy, 128>;

        FHashMap<FInstanceHandle, IntrusivePtr<FRenderInstance>> instances;

        // - Vulkan Data -
		
		VkInstance m_VulkanInstance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_VkMessenger = VK_NULL_HANDLE;

		// - Physical Device -

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties{};
        VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures{};
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

        // - VMA Allocator -

        VmaAllocator m_Allocator = VK_NULL_HANDLE;

        // - Command Pools & Buffers -

        VkCommandPool m_CommandPool = VK_NULL_HANDLE;

        // - SwapChain -

        FHashMap<FWindowHandle, IPtr<FSwapChain>> m_SwapChainsByWindowHandle;

        // - Render Targets -

        FRenderTargetHandle::IndexType m_RenderTargetIndexAllocator = 0;
        FHashMap<FRenderTargetHandle, IPtr<FRenderTarget>> m_RenderTargetsByHandle;

        // - Render Pass -

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;

        // - Pipeline -

        IPtr<FGraphicsPipeline> m_MainGraphicsPipeline;

        // - Per Frame Rendering Resources -

        u32 m_FrameSlot = 0;
        
        FArray<FDescriptorPool*, kImageCount> m_PoolsPerFrame;
        FStaticArray<IPtr<FMappedBuffer>, kImageCount> m_UIDrawDataBuffers;

        FArray<VkCommandBuffer, kImageCount> m_CommandBuffers;
        FArray<VkSemaphore, kImageCount> m_RenderFinishedSemaphores;
        FArray<VkFence, kImageCount> m_RenderFinishedFences;

        FArray<FDeferredDestruction> m_DeferredDestruction;

        // - Transient Resources -

        FArray<FDrawDataBufferViews> m_OffsetDataPerSnapshot;
        FStaticArray<IPtr<FMappedBuffer>, kImageCount> m_StagingBuffers;

        FBufferImageCopyArray m_BufferImageCopies;

        // - Upload Arena -

        FArena m_UploadArena;

        // - Null Buffer -

        FBuffer* m_NullBuffer = nullptr;
        VkDescriptorBufferInfo m_NullBufferInfo{};

        // - Atlas Resources -

        struct FAtlasUploadRegion
        {
            FAtlasHandle Handle;
            u32 Layer = 0;
            FVec2i Pos, Size;
            SizeT DataOffset = 0;
            SizeT DataSize = 0;

            // Upload Data
            SizeT MappedDataOffset = 0;
        };

        FAtlasHandle::IndexType m_AtlasIndexAllocator = 0;
        FHashMap<FAtlasHandle, IPtr<FTextureAtlas>> m_AtlasesByHandle;
        FHashMap<FAtlasHandle, FArray<FAtlasUploadRegion>> m_PendingAtlasUploads;
    };

} // namespace Fusion

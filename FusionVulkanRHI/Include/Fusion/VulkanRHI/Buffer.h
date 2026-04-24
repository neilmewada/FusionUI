#pragma once

namespace Fusion::Vulkan
{
    class FVulkanRenderBackend;

    struct FBufferView
    {
        SizeT StartOffset = 0;
        SizeT ByteSize = 0;
    };

    struct FDrawDataBufferViews
    {
        FInstanceHandle Instance;
        FRenderTargetHandle RenderTarget;

        FBufferView VertexBuffer;
        FBufferView IndexBuffer;
        FBufferView ViewData;

        TArray<FBufferView> LayerTransformBuffers;
        TArray<FBufferView> DrawItemBuffers;
        TArray<FBufferView> ClipRectsBuffers;
        TArray<FBufferView> GradientStopBuffers;

        VkDescriptorSet ViewDataSet = VK_NULL_HANDLE;
        TArray<VkDescriptorSet> LayerTransformSets;
        TArray<VkDescriptorSet> DrawDataSets;
    };
    
    class FUSIONVULKANRHI_API FMappedBuffer : public FIntrusiveBase
    {
    public:

        static constexpr VkDeviceSize AlignUp(VkDeviceSize offset, VkDeviceSize alignment)
        {
            return (offset + alignment - 1) & ~(alignment - 1);
        }

        FMappedBuffer(FVulkanRenderBackend* backend, VkDeviceSize initialSize, VkDeviceSize growSize);

        ~FMappedBuffer();

        void EnsureCapacity(VkDeviceSize capacity);

        void DeferredDestroy();

        VkBufferCreateInfo m_BufferCI{};
        VmaAllocationCreateInfo m_AllocCI{};

        FVulkanRenderBackend* m_Backend = nullptr;
        VkDevice m_Device = VK_NULL_HANDLE;

        VmaAllocation m_Allocation = VK_NULL_HANDLE;

        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VkDeviceSize m_BufferSize = 0;
        VkDeviceSize m_GrowSize = 0;

        u8* m_MappedData = nullptr;

    };

    class FUSIONVULKANRHI_API FBuffer
    {
    public:

        FBuffer(FVulkanRenderBackend* backend, VkDeviceSize bufferSize);

        ~FBuffer();

        FVulkanRenderBackend* m_Backend = nullptr;
        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VkDeviceSize m_BufferSize = 0;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
    };

} // namespace Fusion::Vulkan

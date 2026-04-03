#pragma once

namespace Fusion::Vulkan
{
    class FVulkanRenderBackend;

    struct FBufferView
    {
        SizeT StartOffset = 0;
        SizeT ByteSize = 0;
    };
    
    class FUSIONVULKANRHI_API FUIDrawBuffer : public FIntrusiveBase
    {
    public:

        static constexpr VkDeviceSize AlignUp(VkDeviceSize offset, VkDeviceSize alignment)
        {
            return (offset + alignment - 1) & ~(alignment - 1);
        }

        FUIDrawBuffer(FVulkanRenderBackend* backend, VkDeviceSize initialSize, VkDeviceSize growSize);

        ~FUIDrawBuffer();

        void DeferredDestroy();

        VmaAllocationCreateInfo m_AllocCI{};

        FVulkanRenderBackend* m_Backend = nullptr;
        VkDevice m_Device = VK_NULL_HANDLE;

        VmaAllocation m_Allocation = VK_NULL_HANDLE;

        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VkDeviceSize m_BufferSize = 0;
        VkDeviceSize m_GrowSize = 0;

        u8* m_MappedData = nullptr;
    };

} // namespace Fusion::Vulkan

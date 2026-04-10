#pragma once

namespace Fusion::Vulkan
{
    inline u32 GetFormatBytesPerTexel(VkFormat format)
    {
        switch (format)
        {
        case VK_FORMAT_R8_UNORM:            return 1;
        case VK_FORMAT_R8G8_UNORM:          return 2;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:       return 4;
        case VK_FORMAT_R16_SFLOAT:          return 2;
        case VK_FORMAT_R16G16B16A16_SFLOAT: return 8;
        case VK_FORMAT_R32_SFLOAT:          return 4;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return 16;
        default:
            FUSION_ASSERT(false, "Unsupported format");
            return 0;
        }
    }

    // - Image -

    struct FUSIONVULKANRHI_API FTexture : FIntrusiveBase
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

        FArray<VmaAllocation> m_Allocations;
        FArray<VmaAllocationInfo> m_AllocationInfos;
    };

    // - Atlas -

    class FTextureAtlas : public FIntrusiveBase
    {
    public:

        FTextureAtlas(FVulkanRenderBackend* backend, u32 size, u32 layerCount, VkFormat format);

        ~FTextureAtlas();

        void DeferredDestroy();

        void Create();

        FVulkanRenderBackend* m_Backend = nullptr;
        VkDevice m_Device = VK_NULL_HANDLE;

        VkImageCreateInfo m_ImageCI{};
        VkImageViewCreateInfo m_ImageViewCI{};
        VmaAllocationCreateInfo m_AllocCI{};

        VmaAllocationInfo m_AllocationInfo{};
        VmaAllocation m_Allocation = nullptr;
        VkImage m_Image = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;

        u32 m_Size = 0;
        u32 m_LayerCount = 0;
        VkFormat m_Format = VK_FORMAT_UNDEFINED;
        VkImageLayout m_CurLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImageSubresourceRange m_SubresourceRange{};
    };
    
} // namespace Fusion::Vulkan

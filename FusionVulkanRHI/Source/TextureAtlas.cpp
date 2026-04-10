#include "Fusion/VulkanRHI.h"

namespace Fusion::Vulkan
{
	// -----------------------------------------------------------------
	// Texture
	// -----------------------------------------------------------------

	FTexture::~FTexture()
	{
		VkImage image = !m_IsExternalImage ? m_Image : VK_NULL_HANDLE;
		VkImageView imageView = m_ImageView;
		VkDevice device = m_Device;
		FVulkanRenderBackend* backend = m_RenderBackend;

		if (imageView || image || !m_Allocations.Empty())
		{
			auto allocations = m_Allocations;

			m_RenderBackend->DeferDestruction([image, imageView, device, allocations, backend]
				{
					if (imageView)
						vkDestroyImageView(device, imageView, VULKAN_CPU_ALLOCATOR);
					if (image)
						vkDestroyImage(device, image, VULKAN_CPU_ALLOCATOR);

					for (VmaAllocation allocation : allocations)
					{
						vmaFreeMemory(backend->GetVmaAllocator(), allocation);
					}
				});
		}

		m_IsExternalImage = false;
		m_Image = VK_NULL_HANDLE;
		m_ImageView = VK_NULL_HANDLE;
	}

	// -----------------------------------------------------------------
	// Texture Atlas
	// -----------------------------------------------------------------

	FTextureAtlas::FTextureAtlas(FVulkanRenderBackend* backend, u32 size, u32 layerCount, VkFormat format)
		: m_Backend(backend), m_Device(backend->GetVkDevice()), m_Size(size), m_LayerCount(layerCount), m_Format(format)
	{
		FUSION_ASSERT(layerCount > 0, "FTextureAtlas: invalid layerCount");
		FUSION_ASSERT(size > 0, "FTextureAtlas: invalid size");

		// Image CI

		m_ImageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		m_ImageCI.format = format;
		m_ImageCI.imageType = VK_IMAGE_TYPE_2D;
		m_ImageCI.extent.depth = 1.0f;
		m_ImageCI.extent.width = size;
		m_ImageCI.extent.height = size;
		m_ImageCI.mipLevels = 1;
		m_ImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		m_ImageCI.arrayLayers = layerCount;
		m_ImageCI.initialLayout = m_CurLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_ImageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		
		// Image View CI

		m_ImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		m_ImageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		m_ImageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		m_ImageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		m_ImageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		m_ImageViewCI.format = format;
		m_ImageViewCI.subresourceRange.baseArrayLayer = 0;
		m_ImageViewCI.subresourceRange.layerCount = layerCount;
		m_ImageViewCI.subresourceRange.baseArrayLayer = 0;
		m_ImageViewCI.subresourceRange.levelCount = 1;
		m_ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		m_ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;

		m_SubresourceRange = m_ImageViewCI.subresourceRange;

		VkResult result = vmaCreateImage(m_Backend->GetVmaAllocator(), &m_ImageCI, &m_AllocCI, &m_Image, &m_Allocation, &m_AllocationInfo);
		VULKAN_CHECK(result, "FTextureAtlas failed to vmaCreateImage");

		m_ImageViewCI.image = m_Image;

		result = vkCreateImageView(m_Device, &m_ImageViewCI, VULKAN_CPU_ALLOCATOR, &m_ImageView);
		VULKAN_CHECK(result, "FTextureAtlas failed to create VkImageView.");
	}

	FTextureAtlas::~FTextureAtlas()
	{
		vmaDestroyImage(m_Backend->GetVmaAllocator(), m_Image, m_Allocation);
		
		vkDestroyImageView(m_Device, m_ImageView, VULKAN_CPU_ALLOCATOR);
	}

	void FTextureAtlas::DeferredDestroy()
	{
		VmaAllocator allocator = m_Backend->GetVmaAllocator();
		VkImage image = m_Image;
		VkImageView imageView = m_ImageView;
		VkDevice device = m_Device;
		VmaAllocation allocation = m_Allocation;

		m_Backend->DeferDestruction([allocator, image, imageView, device, allocation]
		{
			vmaDestroyImage(allocator, image, allocation);

			vkDestroyImageView(device, imageView, VULKAN_CPU_ALLOCATOR);
		});

		m_AllocationInfo = {};
		m_ImageView = VK_NULL_HANDLE;
		m_Image = VK_NULL_HANDLE;
		m_Allocation = VK_NULL_HANDLE;
	}

	void FTextureAtlas::Create()
	{
		if (m_Image != VK_NULL_HANDLE)
		{
			DeferredDestroy();
		}

		VkResult result = vmaCreateImage(m_Backend->GetVmaAllocator(), &m_ImageCI, &m_AllocCI, &m_Image, &m_Allocation, &m_AllocationInfo);
		VULKAN_CHECK(result, "FTextureAtlas failed to vmaCreateImage");

		m_ImageViewCI.image = m_Image;

		result = vkCreateImageView(m_Device, &m_ImageViewCI, VULKAN_CPU_ALLOCATOR, &m_ImageView);
		VULKAN_CHECK(result, "FTextureAtlas failed to create VkImageView.");
	}

} // namespace Fusion::Vulkan

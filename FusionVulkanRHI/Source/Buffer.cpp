#include "VulkanRHIPrivate.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion::Vulkan
{
	FUIDrawBuffer::FUIDrawBuffer(FVulkanRenderBackend* backend, VkDeviceSize initialSize, VkDeviceSize growSize)
		: m_Backend(backend), m_Device(m_Backend->GetVkDevice()), m_BufferSize(initialSize), m_GrowSize(growSize)
	{
		m_BufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		m_BufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		m_BufferCI.size = m_BufferSize;
		
		m_AllocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		m_AllocCI.usage = (m_Backend->IsResizableBAR() || m_Backend->IsUnifiedMemory())
			? VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
			: VMA_MEMORY_USAGE_AUTO_PREFER_HOST;

		VmaAllocationInfo alloc{};
		VkResult result = vmaCreateBuffer(backend->GetVmaAllocator(), &m_BufferCI, &m_AllocCI, &m_Buffer, &m_Allocation, &alloc);
		VULKAN_CHECK(result, "Failed to create VkBuffer using VmaAllocator.");

		m_MappedData = (u8*)alloc.pMappedData;
	}

	FUIDrawBuffer::~FUIDrawBuffer()
	{
		vmaDestroyBuffer(m_Backend->GetVmaAllocator(), m_Buffer, m_Allocation);
	}

	void FUIDrawBuffer::EnsureCapacity(VkDeviceSize capacity)
	{
		if (m_BufferSize < capacity)
		{
			m_BufferCI.size = FMath::Max(capacity, m_BufferSize + m_GrowSize);

			VmaAllocation allocation = VK_NULL_HANDLE;
			VmaAllocationInfo alloc{};
			VkBuffer buffer = VK_NULL_HANDLE;

			VkResult result = vmaCreateBuffer(m_Backend->GetVmaAllocator(), &m_BufferCI, &m_AllocCI, &buffer, &allocation, &alloc);
			VULKAN_CHECK(result, "Failed to grow VkBuffer.");

			memcpy(alloc.pMappedData, m_MappedData, m_BufferSize);

			DeferredDestroy();

			m_BufferSize = capacity;
			m_MappedData = (u8*)alloc.pMappedData;
			m_Buffer = buffer;
			m_Allocation = allocation;
		}
	}

	void FUIDrawBuffer::DeferredDestroy()
	{
		VmaAllocator vmaAllocator = m_Backend->GetVmaAllocator();
		VkBuffer buffer = m_Buffer;
		VmaAllocation allocation = m_Allocation;

		m_Backend->DeferDestruction([vmaAllocator, buffer, allocation]
		{
			vmaDestroyBuffer(vmaAllocator, buffer, allocation);
		});

		m_MappedData = nullptr;
		m_Buffer = VK_NULL_HANDLE;
		m_Allocation = VK_NULL_HANDLE;
	}

	FBuffer::FBuffer(FVulkanRenderBackend* backend, VkDeviceSize bufferSize) : m_Backend(backend)
	{
		VkBufferCreateInfo bufferCI{};
		bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;;
		bufferCI.size = bufferSize;
		m_BufferSize = bufferSize;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.flags = 0;
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		VmaAllocationInfo alloc{};
		VkResult result = vmaCreateBuffer(backend->GetVmaAllocator(), &bufferCI, &allocInfo, &m_Buffer, &m_Allocation, &alloc);
		VULKAN_CHECK(result, "Failed to create vma Buffer");
	}

	FBuffer::~FBuffer()
	{
		vmaDestroyBuffer(m_Backend->GetVmaAllocator(), m_Buffer, m_Allocation);
	}
} // namespace Fusion::Vulkan

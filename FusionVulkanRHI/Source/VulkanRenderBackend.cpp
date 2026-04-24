#include "VulkanRHIPrivate.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "PAL/VulkanPlatform.h"

namespace Fusion::Vulkan
{
	VKAPI_ATTR static VkBool32 VulkanValidationCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		[[maybe_unused]] void* pUserData)
	{
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			FUSION_LOG_ERROR("VulkanValidation", "Vulkan Error: {}", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			FUSION_LOG_WARNING("VulkanValidation", "Vulkan Warning: {}", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			FUSION_LOG_INFO("VulkanValidation", "Vulkan Info: {}", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			FUSION_LOG_TRACE("VulkanValidation", "Vulkan Verbose: {}", pCallbackData->pMessage);
		}
		return VK_FALSE;
	}

	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(instance, debugMessenger, pAllocator);
		}
	}

	static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = VulkanValidationCallback;
	}

	// -----------------------------------------------------------------
	// Descriptor Pool
	// -----------------------------------------------------------------

	FDescriptorPool::~FDescriptorPool()
	{
		VkDevice device = m_Device;

		for (int i = 0; i < m_Pools.Size(); i++)
		{
			VkDescriptorPool pool = m_Pools[i];
			vkDestroyDescriptorPool(device, pool, VULKAN_CPU_ALLOCATOR);
		}

		m_Pools.Clear();
	}

	void FDescriptorPool::Grow()
	{
		TArray<VkDescriptorPoolSize, 10> poolSizes = {
			{.type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 32 },
			{.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = 32 },
			{.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount = 4 },
			{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 32 },
			{.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, .descriptorCount = 2 },
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 2 },
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 32 },
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, .descriptorCount = 2 },
		};

		VkDescriptorPoolCreateInfo poolCI{};
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.maxSets = 128;
		poolCI.poolSizeCount = (uint32_t)poolSizes.Size();
		poolCI.pPoolSizes = poolSizes.Data();
		poolCI.flags = 0;

		VkDescriptorPool pool = VK_NULL_HANDLE;

		auto result = vkCreateDescriptorPool(m_Device, &poolCI, VULKAN_CPU_ALLOCATOR, &pool);
		VULKAN_CHECK(result, "Failed to create VkDescriptorPool");

		m_Pools.Add(pool);
	}

	void FDescriptorPool::Reset()
	{
		for (VkDescriptorPool pool : m_Pools)
		{
			vkResetDescriptorPool(m_Device, pool, 0);
		}

		m_CurPoolIndex = 0;
	}

	VkDescriptorSet FDescriptorPool::Allocate(VkDescriptorSetLayout setLayout)
	{
		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = m_Pools[m_CurPoolIndex];
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &setLayout;

		VkDescriptorSet set = VK_NULL_HANDLE;

		auto result = vkAllocateDescriptorSets(m_Device, &allocateInfo, &set);

		while (result == VK_ERROR_OUT_OF_POOL_MEMORY)
		{
			bool wasIncremented = false;

			if (m_CurPoolIndex < (int)m_Pools.Size() - 1)
			{
				m_CurPoolIndex++;
			}
			else
			{
				Grow();
				m_CurPoolIndex++;
				wasIncremented = true;
			}

			allocateInfo.descriptorPool = m_Pools[m_CurPoolIndex];
			result = vkAllocateDescriptorSets(m_Device, &allocateInfo, &set);

			FUSION_ASSERT(!wasIncremented || result == VK_SUCCESS, "Failed to allocate VkDescriptorSet");
		}

		VULKAN_CHECK(result, "Failed to allocate VkDescriptorSet");

		return set;
	}

	// -----------------------------------------------------------------
	// SwapChain
	// -----------------------------------------------------------------

	FSwapChain::FSwapChain(FVulkanRenderBackend* renderBackend, VkDevice device) 
		: m_RenderBackend(renderBackend), m_Instance(renderBackend->GetVkInstance()), m_Device(device)
	{
		
	}

	FSwapChain::~FSwapChain()
	{
		VkDevice device = m_Device;

		for (int i = 0; i < m_ImageAcquiredSemaphores.Size(); i++)
		{
			VkSemaphore semaphore = m_ImageAcquiredSemaphores[i];
			
			m_RenderBackend->DeferDestruction([device, semaphore]
			{
				vkDestroySemaphore(device, semaphore, VULKAN_CPU_ALLOCATOR);
			});
		}
		m_ImageAcquiredSemaphores.Clear();

		m_Images.Clear();

		for (int i = 0; i < m_FrameBuffers.Size(); i++)
		{
			VkFramebuffer frameBuffer = m_FrameBuffers[i];

			m_RenderBackend->DeferDestruction([device, frameBuffer]
			{
				vkDestroyFramebuffer(device, frameBuffer, VULKAN_CPU_ALLOCATOR);
			});
			
		}
		m_FrameBuffers.Clear();

		VkSwapchainKHR swapChain = m_SwapChain;
		VkSurfaceKHR surface = m_Surface;
		VkInstance instance = m_Instance;

		m_RenderBackend->DeferDestruction([device, surface, swapChain, instance]
		{
			vkDestroySwapchainKHR(device, swapChain, VULKAN_CPU_ALLOCATOR);
			vkDestroySurfaceKHR(instance, surface, VULKAN_CPU_ALLOCATOR);
		});
		
		m_SwapChain = VK_NULL_HANDLE;
		m_Surface = VK_NULL_HANDLE;
	}


	// -----------------------------------------------------------------
	// Graphics Pipeline
	// -----------------------------------------------------------------

	FGraphicsPipeline::FGraphicsPipeline(VkDevice device) : m_Device(device)
	{
	}

	FGraphicsPipeline::~FGraphicsPipeline()
	{
		for (int i = 0; i < m_SetLayouts.Size(); i++)
		{
			if (m_SetLayouts[i])
			{
				vkDestroyDescriptorSetLayout(m_Device, m_SetLayouts[i], VULKAN_CPU_ALLOCATOR);
			}
		}
		m_SetLayouts.Clear();

		vkDestroyPipeline(m_Device, m_Pipeline, VULKAN_CPU_ALLOCATOR);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, VULKAN_CPU_ALLOCATOR);

		for (int i = 0; i < m_ImmutableSamplers.Size(); i++)
		{
			vkDestroySampler(m_Device, m_ImmutableSamplers[i], VULKAN_CPU_ALLOCATOR);
		}
		m_ImmutableSamplers.Clear();

		vkDestroyShaderModule(m_Device, m_VertexModule, VULKAN_CPU_ALLOCATOR);
		vkDestroyShaderModule(m_Device, m_FragmentModule, VULKAN_CPU_ALLOCATOR);
	}


	// -----------------------------------------------------------------
	// Vulkan Render Backend
	// -----------------------------------------------------------------

	FRenderBackendCapabilities FVulkanRenderBackend::GetRenderCapabilities()
	{
		return {
			.MinConstantBufferOffsetAlignment = m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment,
			.MinStructuredBufferOffsetAlignment = m_PhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment
		};
	}

	bool FVulkanRenderBackend::InitializeInstance(FInstanceHandle instance)
	{
		if (m_Instances.KeyExists(instance))
		{
			return true;
		}

		if (m_VulkanInstance == VK_NULL_HANDLE)
		{
			InitializeVulkan();
		}

		IPtr<FRenderInstance> renderInstance = new FRenderInstance();

		m_Instances.Add(instance, renderInstance);

		return true;
	}

	void FVulkanRenderBackend::ShutdownInstance(FInstanceHandle instance)
	{
		if (!m_Instances.KeyExists(instance))
		{
			return;
		}

		m_Instances.Remove(instance);

		if (m_Instances.IsEmpty())
		{
			ShutdownVulkan();
		}
	}

	void FVulkanRenderBackend::SubmitSnapshot(FRenderTargetHandle renderTarget, IPtr<FRenderSnapshot> snapshot)
	{
		auto it = m_RenderTargetsByHandle.Find(renderTarget);
		if (it == m_RenderTargetsByHandle.End())
			return;

		it->second->Snapshot = snapshot;
	}

	FAtlasHandle FVulkanRenderBackend::CreateLayeredAtlas(bool grayscale, u32 resolution, u32 numLayers)
	{
		m_AtlasIndexAllocator += 1;
		FAtlasHandle handle = FAtlasHandle(m_AtlasIndexAllocator);

		IPtr<FTextureAtlas> atlas = new FTextureAtlas(this, resolution, numLayers, grayscale ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM);

		m_AtlasesByHandle[handle] = atlas;

		return handle;
	}

	static SizeT GetTexelBlockSize(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_UNORM:
			return 4;
		}
		return 1;
	}

	void FVulkanRenderBackend::UploadAtlasRegionAsync(FAtlasHandle handle, u32 layer, FVec2i pos, FVec2i size,
		const u8* pixels, int pitch)
	{
		auto it = m_AtlasesByHandle.Find(handle);
		if (it == m_AtlasesByHandle.End())
			return;

		IPtr<FTextureAtlas> atlas = it->second;
		if (!atlas)
			return;

		u32 bytesPerPixel = GetFormatBytesPerTexel(atlas->m_Format);
		if (bytesPerPixel == 0)
			return;

		u32 bytesPerRow = size.x * bytesPerPixel;
		SizeT byteOffset = m_UploadArena.GetByteSize();
		SizeT byteCount = (SizeT)size.y * bytesPerRow;

		for (int row = 0; row < size.y; row++)
		{
			m_UploadArena.InsertRange(pixels + row * pitch, bytesPerRow);
		}

		m_PendingAtlasUploads[handle].Add({
			.Handle = handle,
			.Layer = layer,
			.Pos = pos,
			.Size = size,
			.DataOffset = byteOffset,
			.DataSize = byteCount
		});
	}

	void FVulkanRenderBackend::SetFontAtlas(FInstanceHandle instance, FAtlasHandle atlas)
	{
		if (!IsInitialized(instance))
			return;

		m_Instances[instance]->FontAtlas = atlas;
	}

	void FVulkanRenderBackend::SetImageAtlas(FInstanceHandle instance, FAtlasHandle atlas)
	{
		if (!IsInitialized(instance))
			return;

		m_Instances[instance]->ImageAtlas = atlas;
	}

	void FVulkanRenderBackend::DestroyAtlas(FAtlasHandle atlas)
	{
		for (auto [instanceHandle, instance] : m_Instances)
		{
			if (instance->FontAtlas == atlas)
			{
				instance->FontAtlas = {};
			}
		}

		auto it = m_AtlasesByHandle.Find(atlas);
		if (it != m_AtlasesByHandle.End())
		{
			it->second->DeferredDestroy();
			m_AtlasesByHandle.Remove(atlas);
		}
	}

	FRenderTargetHandle FVulkanRenderBackend::AcquireWindowRenderTarget(FInstanceHandle instance, FWindowHandle window)
	{
		if (window.IsNull())
			return {};
		if (!m_SwapChainsByWindowHandle.KeyExists(window))
			return {};
		if (m_SwapChainsByWindowHandle[window] == nullptr)
			return {};

		m_RenderTargetIndexAllocator += 1;
		auto handle = FRenderTargetHandle(m_RenderTargetIndexAllocator);

		IPtr<FRenderTarget> renderTarget = new FRenderTarget;
		renderTarget->Instance = instance;
		renderTarget->Type = ERenderTargetType::Window;
		renderTarget->Window = window;

		m_RenderTargetsByHandle[handle] = renderTarget;

		return handle;
	}

	void FVulkanRenderBackend::ReleaseRenderTarget(FInstanceHandle instance, FRenderTargetHandle renderTarget)
	{
		auto it = m_RenderTargetsByHandle.Find(renderTarget);

		if (it == m_RenderTargetsByHandle.End())
			return;

		m_RenderTargetsByHandle.Remove(it);
	}

	void FVulkanRenderBackend::TickDestructionQueue()
	{
		for (int i = (int)m_DeferredDestruction.Size() - 1; i >= 0; --i)
		{
			if (m_DeferredDestruction[i].m_FrameCounter <= 0)
			{
				m_DeferredDestruction[i].m_Destruction.ExecuteIfBound();
				m_DeferredDestruction.RemoveAtSwapLast(i);
				continue;
			}

			m_DeferredDestruction[i].m_FrameCounter--;
		}
	}

	// ------------------------------------------------------------------------------------------------------------
	// - Render Tick
	

	void FVulkanRenderBackend::RenderTick()
	{
		if (m_SwapChainsByWindowHandle.IsEmpty())
			return;

		ZoneScoped;

		VkResult result = VK_SUCCESS;

		SizeT curUploadOffset = 0;

		constexpr uint64_t kSwapChainTimeOut = UINT64_MAX;
		constexpr uint64_t kFenceTimeOut = UINT64_MAX;

		TArray<VkSemaphore> waitSemaphores;
		TArray<VkPipelineStageFlags> waitSemaphoreStages;

		TArray<VkSwapchainKHR> presentSwapChains{};
		TArray<uint32_t> presentImageIndices{};

		// - Wait for GPU -

		result = vkWaitForFences(m_Device, 1, &m_RenderFinishedFences[m_FrameSlot], VK_TRUE, kFenceTimeOut);
		VULKAN_CHECK(result, "Failed to wait on Render Finished Fence.");

		vkResetFences(m_Device, 1, &m_RenderFinishedFences[m_FrameSlot]);

		// - Reset Descriptor Pool -

		m_PoolsPerFrame[m_FrameSlot]->Reset();
		FDescriptorPool* pool = m_PoolsPerFrame[m_FrameSlot];

		// - Prepare Draw Buffer Offsets -

		m_OffsetDataPerSnapshot.Clear();
		VkDeviceSize currentDrawBufferOffset = 0;
		VkDeviceSize alignment = FMath::Max(m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment, m_PhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment);

		for (auto [renderTargetHandle, renderTarget] : m_RenderTargetsByHandle)
		{
			if (!renderTarget->Snapshot)
				continue;

			auto snapshot = renderTarget->Snapshot;

			if (currentDrawBufferOffset > 0)
				currentDrawBufferOffset = FMappedBuffer::AlignUp(currentDrawBufferOffset, alignment);
			
			FDrawDataBufferViews views{};

			views.Instance = renderTarget->Instance;
			views.RenderTarget = renderTargetHandle;

			views.VertexBuffer.StartOffset = currentDrawBufferOffset;
			views.VertexBuffer.ByteSize = snapshot->vertexArray.GetByteSize();
			currentDrawBufferOffset += views.VertexBuffer.ByteSize;

			views.IndexBuffer.StartOffset = currentDrawBufferOffset;
			views.IndexBuffer.ByteSize = snapshot->indexArray.GetByteSize();
			currentDrawBufferOffset += views.IndexBuffer.ByteSize;

			currentDrawBufferOffset = FMappedBuffer::AlignUp(currentDrawBufferOffset, alignment);
			views.ViewData.StartOffset = currentDrawBufferOffset;
			views.ViewData.ByteSize = sizeof(snapshot->viewData);
			currentDrawBufferOffset += views.ViewData.ByteSize;

			for (int i = 0; i < snapshot->transformMatricesPerLayer.GetCount(); i++)
			{
				currentDrawBufferOffset = FMappedBuffer::AlignUp(currentDrawBufferOffset, alignment);

				views.LayerTransformBuffers.Add({
					.StartOffset = currentDrawBufferOffset,
					.ByteSize = sizeof(FMat4)
				});

				currentDrawBufferOffset += sizeof(FMat4);
			}

			for (int i = 0; i < snapshot->drawItemSplits.GetCount(); i++)
			{
				currentDrawBufferOffset = FMappedBuffer::AlignUp(currentDrawBufferOffset, alignment);

				views.DrawItemBuffers.Add({
					.StartOffset = currentDrawBufferOffset,
					.ByteSize = snapshot->drawItemSplits[i].ByteSize
				});

				currentDrawBufferOffset += snapshot->drawItemSplits[i].ByteSize;
			}

			for (int i = 0; i < snapshot->clipRectSplits.GetCount(); i++)
			{
				currentDrawBufferOffset = FMappedBuffer::AlignUp(currentDrawBufferOffset, alignment);

				views.ClipRectsBuffers.Add({
					.StartOffset = currentDrawBufferOffset,
					.ByteSize = snapshot->clipRectSplits[i].ByteSize
				});

				currentDrawBufferOffset += snapshot->clipRectSplits[i].ByteSize;
			}

			for (int i = 0; i < snapshot->gradientStopSplits.GetCount(); i++)
			{
				currentDrawBufferOffset = FMappedBuffer::AlignUp(currentDrawBufferOffset, alignment);

				views.GradientStopBuffers.Add({
					.StartOffset = currentDrawBufferOffset,
					.ByteSize = snapshot->gradientStopSplits[i].ByteSize
				});

				currentDrawBufferOffset += snapshot->gradientStopSplits[i].ByteSize;
			}

			m_OffsetDataPerSnapshot.Add(views);
		}

		FMappedBuffer* drawBuffer = m_UIDrawDataBuffers[m_FrameSlot];

		drawBuffer->EnsureCapacity(currentDrawBufferOffset);
		u8* dataPtr = drawBuffer->m_MappedData;

		// - Upload Draw Buffer Data -

		for (SizeT i = 0; i < m_OffsetDataPerSnapshot.Size(); i++)
		{
			FDrawDataBufferViews& views = m_OffsetDataPerSnapshot[i];
			IPtr<FRenderTarget> renderTarget = m_RenderTargetsByHandle[views.RenderTarget];
			IPtr<FRenderSnapshot> snapshot = renderTarget->Snapshot;

			VkDescriptorSetLayout viewDataSetLayout = m_MainPipeline->m_SetLayouts[kViewDataSetIndex];
			VkDescriptorSetLayout layerTransformsSetLayout = m_MainPipeline->m_SetLayouts[kLayerTransformsSetIndex];
			VkDescriptorSetLayout drawDataSetLayout = m_MainPipeline->m_SetLayouts[kDrawDataSetIndex];

			views.ViewDataSet = pool->Allocate(viewDataSetLayout);

			// Vertex & Index Data

			if (views.VertexBuffer.ByteSize > 0)
			{
				memcpy(dataPtr + views.VertexBuffer.StartOffset, snapshot->vertexArray.GetData(), views.VertexBuffer.ByteSize);
			}

			if (views.IndexBuffer.ByteSize > 0)
			{
				memcpy(dataPtr + views.IndexBuffer.StartOffset, snapshot->indexArray.GetData(), views.IndexBuffer.ByteSize);
			}

			// View Data Set
			{
				VkDescriptorBufferInfo viewDataBufferInfo{
					.buffer = drawBuffer->m_Buffer,
					.offset = views.ViewData.StartOffset,
					.range = views.ViewData.ByteSize
				};

				VkWriteDescriptorSet viewDataBufferWrite{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = views.ViewDataSet,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pImageInfo = nullptr,
					.pBufferInfo = &m_NullBufferInfo,
					.pTexelBufferView = nullptr
				};

				if (views.ViewData.ByteSize > 0)
				{
					memcpy(dataPtr + views.ViewData.StartOffset, (void*)&snapshot->viewData, views.ViewData.ByteSize);

					viewDataBufferWrite.pBufferInfo = &viewDataBufferInfo;
				}

				vkUpdateDescriptorSets(m_Device, 1, &viewDataBufferWrite, 0, nullptr);
			}

			// Draw Data Set
			{
				views.DrawDataSets.Resize(views.DrawItemBuffers.Size());

				TArray<VkDescriptorBufferInfo> bufferInfos(views.DrawItemBuffers.Size() * 3);
				TArray<VkWriteDescriptorSet> setWrites(views.DrawItemBuffers.Size() * 3);

				for (SizeT layerIdx = 0; layerIdx < views.DrawItemBuffers.Size(); layerIdx++)
				{
					views.DrawDataSets[layerIdx] = pool->Allocate(drawDataSetLayout);

					SizeT setIdx = layerIdx * 3;

					// - Draw Items -

					bufferInfos[setIdx] = {
						.buffer = drawBuffer->m_Buffer,
						.offset = views.DrawItemBuffers[layerIdx].StartOffset,
						.range = views.DrawItemBuffers[layerIdx].ByteSize
					};

					setWrites[setIdx] = {
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.pNext = nullptr,
						.dstSet = views.DrawDataSets[layerIdx],
						.dstBinding = 0,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						.pImageInfo = nullptr,
						.pBufferInfo = &m_NullBufferInfo,
						.pTexelBufferView = nullptr
					};

					// - Clip Rects -

					bufferInfos[setIdx + 1] = {
						.buffer = drawBuffer->m_Buffer,
						.offset = views.ClipRectsBuffers[layerIdx].StartOffset,
						.range = views.ClipRectsBuffers[layerIdx].ByteSize
					};

					setWrites[setIdx + 1] = {
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.pNext = nullptr,
						.dstSet = views.DrawDataSets[layerIdx],
						.dstBinding = 1,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						.pImageInfo = nullptr,
						.pBufferInfo = &m_NullBufferInfo,
						.pTexelBufferView = nullptr
					};

					// - Gradient Stops -

					bufferInfos[setIdx + 2] = {
						.buffer = drawBuffer->m_Buffer,
						.offset = views.GradientStopBuffers[layerIdx].StartOffset,
						.range = views.GradientStopBuffers[layerIdx].ByteSize
					};

					setWrites[setIdx + 2] = { 
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.pNext = nullptr,
						.dstSet = views.DrawDataSets[layerIdx],
						.dstBinding = 2,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						.pImageInfo = nullptr,
						.pBufferInfo = &m_NullBufferInfo,
						.pTexelBufferView = nullptr
					};

					if (views.DrawItemBuffers[layerIdx].ByteSize > 0)
					{
						memcpy(dataPtr + views.DrawItemBuffers[layerIdx].StartOffset, (u8*)snapshot->drawItemArray.GetData() + snapshot->drawItemSplits[layerIdx].StartOffset, views.DrawItemBuffers[layerIdx].ByteSize);

						setWrites[setIdx].pBufferInfo = &bufferInfos[setIdx];
					}

					if (views.ClipRectsBuffers[layerIdx].ByteSize > 0)
					{
						memcpy(dataPtr + views.ClipRectsBuffers[layerIdx].StartOffset, (u8*)snapshot->clipRectArray.GetData() + snapshot->clipRectSplits[layerIdx].StartOffset, views.ClipRectsBuffers[layerIdx].ByteSize);

						setWrites[setIdx + 1].pBufferInfo = &bufferInfos[setIdx + 1];
					}

					if (views.GradientStopBuffers[layerIdx].ByteSize > 0)
					{
						memcpy(dataPtr + views.GradientStopBuffers[layerIdx].StartOffset, (u8*)snapshot->gradientStopArray.GetData() + snapshot->gradientStopSplits[layerIdx].StartOffset, views.GradientStopBuffers[layerIdx].ByteSize);

						setWrites[setIdx + 2].pBufferInfo = &bufferInfos[setIdx + 2];
					}
				}

				vkUpdateDescriptorSets(m_Device, (uint32_t)setWrites.Count(), setWrites.Data(), 0, nullptr);
			}

			// Layer Transforms

			{
				views.LayerTransformSets.Resize(views.LayerTransformBuffers.Size());

				TArray<VkDescriptorBufferInfo> bufferInfos(views.LayerTransformBuffers.Size());
				TArray<VkWriteDescriptorSet> setWrites(views.LayerTransformBuffers.Size());

				for (SizeT j = 0; j < views.LayerTransformBuffers.Size(); j++)
				{
					views.LayerTransformSets[j] = pool->Allocate(layerTransformsSetLayout);

					bufferInfos[j] = {
						.buffer = drawBuffer->m_Buffer,
						.offset = views.LayerTransformBuffers[j].StartOffset,
						.range = views.LayerTransformBuffers[j].ByteSize
					};

					setWrites[j] = {
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.pNext = nullptr,
						.dstSet = views.LayerTransformSets[j],
						.dstBinding = 0,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						.pImageInfo = nullptr,
						.pBufferInfo = &m_NullBufferInfo,
						.pTexelBufferView = nullptr
					};

					if (views.LayerTransformBuffers[j].ByteSize > 0)
					{
						memcpy(dataPtr + views.LayerTransformBuffers[j].StartOffset, (void*)&snapshot->transformMatricesPerLayer[j], views.LayerTransformBuffers[j].ByteSize);

						setWrites[j].pBufferInfo = &bufferInfos[j];
					}
				}

				vkUpdateDescriptorSets(m_Device, (uint32_t)setWrites.Count(), setWrites.Data(), 0, nullptr);
			}
		}

		// - Upload Atlas Regions -

		bool atlasUploadRequired = false;

		for (auto& [atlasHandle, atlasUploadRegions]: m_PendingAtlasUploads)
		{
			auto it = m_AtlasesByHandle.Find(atlasHandle);
			if (it == m_AtlasesByHandle.End())
				continue;

			for (auto& atlasUploadRegion : atlasUploadRegions)
			{
				SizeT blockSize = GetTexelBlockSize(it->second->m_Format);
				if (blockSize > 1)
				{
					curUploadOffset = FMemoryUtils::AlignUp(curUploadOffset, blockSize);
				}

				m_StagingBuffers[m_FrameSlot]->EnsureCapacity(curUploadOffset + atlasUploadRegion.DataSize);

				u8* dstData = m_StagingBuffers[m_FrameSlot]->m_MappedData + curUploadOffset;
				memcpy(dstData, m_UploadArena.GetData() + atlasUploadRegion.DataOffset, atlasUploadRegion.DataSize);

				atlasUploadRegion.MappedDataOffset = curUploadOffset;

				curUploadOffset += atlasUploadRegion.DataSize;

				atlasUploadRequired = true;
			}
		}

		// - Global Set -

		for (auto [instanceHandle, instance] : m_Instances)
		{
			instance->GlobalSet = VK_NULL_HANDLE;

			FAtlasHandle fontAtlasHandle = instance->FontAtlas;
			if (fontAtlasHandle.IsNull())
				continue;
			
			FAtlasHandle imageAtlasHandle = instance->ImageAtlas;
			if (imageAtlasHandle.IsNull())
				continue;

			auto it = m_AtlasesByHandle.Find(fontAtlasHandle);
			if (it == m_AtlasesByHandle.End())
				continue;

			auto it2 = m_AtlasesByHandle.Find(imageAtlasHandle);
			if (it2 == m_AtlasesByHandle.End())
				continue;

			IPtr<FTextureAtlas> fontAtlas = it->second;
			IPtr<FTextureAtlas> imageAtlas = it2->second;

			instance->GlobalSet = m_PoolsPerFrame[m_FrameSlot]->Allocate(m_MainPipeline->m_SetLayouts[kGlobalSetIndex]);

			FStaticArray<VkDescriptorImageInfo, 2> imageInfos{};
			FStaticArray<VkWriteDescriptorSet, 2> writeSets{};

			VkDescriptorImageInfo& imageAtlasInfo = imageInfos[0];
			imageAtlasInfo.imageView = imageAtlas->m_ImageView;
			imageAtlasInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet& imageAtlasBinding = writeSets[0];
			imageAtlasBinding.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			imageAtlasBinding.descriptorCount = 1;
			imageAtlasBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			imageAtlasBinding.dstArrayElement = 0;
			imageAtlasBinding.dstBinding = 1;
			imageAtlasBinding.dstSet = instance->GlobalSet;
			imageAtlasBinding.pImageInfo = &imageAtlasInfo;

			VkDescriptorImageInfo& fontAtlasInfo = imageInfos[1];
			fontAtlasInfo.imageView = fontAtlas->m_ImageView;
			fontAtlasInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet& fontAtlasBinding = writeSets[1];
			fontAtlasBinding.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			fontAtlasBinding.descriptorCount = 1;
			fontAtlasBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			fontAtlasBinding.dstArrayElement = 0;
			fontAtlasBinding.dstBinding = 2;
			fontAtlasBinding.dstSet = instance->GlobalSet;
			fontAtlasBinding.pImageInfo = &fontAtlasInfo;
			
			vkUpdateDescriptorSets(m_Device, writeSets.Size(), writeSets.Data(), 0, nullptr);
		}

		// - Acquire SwapChain -

		for (auto [windowHandle, swapChain] : m_SwapChainsByWindowHandle)
		{
			if (!swapChain)
				continue;

			result = vkAcquireNextImageKHR(m_Device, swapChain->m_SwapChain, kSwapChainTimeOut,
				swapChain->m_ImageAcquiredSemaphores[m_FrameSlot], VK_NULL_HANDLE, &swapChain->m_CurrentImageIndex);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			{
				CreateOrUpdateSwapChain(windowHandle);

				result = vkAcquireNextImageKHR(m_Device, swapChain->m_SwapChain, kSwapChainTimeOut,
					swapChain->m_ImageAcquiredSemaphores[m_FrameSlot], VK_NULL_HANDLE, &swapChain->m_CurrentImageIndex);
			}

			VULKAN_CHECK(result, "Failed to acquire image from SwapChain.");

			waitSemaphores.Add(swapChain->m_ImageAcquiredSemaphores[m_FrameSlot]);
			waitSemaphoreStages.Add(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

			presentSwapChains.Add(swapChain->m_SwapChain);
			presentImageIndices.Add(swapChain->m_CurrentImageIndex);
		}

		// - Tick Destruction -

		TickDestructionQueue();

		VkCommandBuffer cmdBuffer = m_CommandBuffers[m_FrameSlot];

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		int numRenderPasses = 0;

		// - Command Buffer Recording -

		vkBeginCommandBuffer(cmdBuffer, &beginInfo);
		{
			// - Uploads -

			if (atlasUploadRequired)
			{
				for (auto& [atlasHandle, atlasUploadRegions] : m_PendingAtlasUploads)
				{
					auto it = m_AtlasesByHandle.Find(atlasHandle);
					if (it == m_AtlasesByHandle.End())
						continue;

					IPtr<FTextureAtlas> atlas = it->second;
                    
                    if (!atlas->m_HasBeenCleared)
                    {
                        atlas->m_HasBeenCleared = true;
                        
                        VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 0.0f } };
                        
                        VkImageSubresourceRange range{};
                        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        range.baseArrayLayer = 0;
                        range.layerCount = atlas->m_LayerCount;
                        range.baseMipLevel = 0;
                        range.levelCount = 1;

						TransitionImageLayout(cmdBuffer, atlas->m_Image, atlas->m_CurLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
						atlas->m_CurLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                        
                        vkCmdClearColorImage(cmdBuffer, atlas->m_Image, atlas->m_CurLayout, &clearColor, 1, &range);
                    }

					m_BufferImageCopies.RemoveAll();

					for (const auto& atlasUploadRegion : atlasUploadRegions)
					{
						VkBufferImageCopy copy{};
						copy.imageSubresource.baseArrayLayer = atlasUploadRegion.Layer;
						copy.imageSubresource.layerCount = 1;
						copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copy.imageSubresource.mipLevel = 0;
						copy.imageOffset.x = atlasUploadRegion.Pos.x;
						copy.imageOffset.y = atlasUploadRegion.Pos.y;
						copy.imageOffset.z = 0;
						copy.imageExtent.width = atlasUploadRegion.Size.width;
						copy.imageExtent.height = atlasUploadRegion.Size.height;
						copy.imageExtent.depth = 1.0f;
						copy.bufferOffset = atlasUploadRegion.MappedDataOffset;
						copy.bufferRowLength = 0;
						copy.bufferImageHeight = 0;

						m_BufferImageCopies.Insert(copy);
					}

					TransitionImageLayout(cmdBuffer, atlas->m_Image, atlas->m_CurLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, atlas->m_SubresourceRange);
					atlas->m_CurLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

					vkCmdCopyBufferToImage(cmdBuffer, m_StagingBuffers[m_FrameSlot]->m_Buffer, atlas->m_Image, atlas->m_CurLayout, m_BufferImageCopies.GetCount(), m_BufferImageCopies.GetData());

					TransitionImageLayout(cmdBuffer, atlas->m_Image, atlas->m_CurLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, atlas->m_SubresourceRange);
					atlas->m_CurLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
			}

			// - Render Passes -

			for (SizeT snapshotIdx = 0; snapshotIdx < m_OffsetDataPerSnapshot.Size(); snapshotIdx++)
			{
				VkClearValue colorClear;
				colorClear.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

				const FDrawDataBufferViews& views = m_OffsetDataPerSnapshot[snapshotIdx];

				if (views.Instance.IsNull())
					continue;

				IPtr<FRenderInstance> instance = nullptr;
				if (!m_Instances.TryGet(views.Instance, instance) || instance == nullptr)
					continue;

				IPtr<FRenderTarget> renderTarget = m_RenderTargetsByHandle[views.RenderTarget];
				if (renderTarget->Type != ERenderTargetType::Window)
					continue;

				IPtr<FRenderSnapshot> snapshot = renderTarget->Snapshot;
				if (!snapshot)
					continue;

				IPtr<FSwapChain> swapChain = m_SwapChainsByWindowHandle[renderTarget->Window];
				if (!swapChain)
					continue;

				VkRenderPassBeginInfo renderPassInfo{};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassInfo.clearValueCount = 1;
				renderPassInfo.pClearValues = &colorClear;
				renderPassInfo.framebuffer = swapChain->m_FrameBuffers[swapChain->m_CurrentImageIndex];
				renderPassInfo.renderArea = {
					.offset = { .x = 0, .y = 0 },
					.extent = { .width = swapChain->m_Width, .height = swapChain->m_Height }
				};
				renderPassInfo.renderPass = m_RenderPass;

				vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
				{
					const VkViewport viewport{
						.x = 0,
						.y = 0,
						.width = (float)swapChain->m_Width,
						.height = (float)swapChain->m_Height,
						.minDepth = 0.0f,
						.maxDepth = 1.0f
					};
					vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

					const VkRect2D scissor{
						.offset = {.x = 0, .y = 0 },
						.extent = {.width = swapChain->m_Width, .height = swapChain->m_Height }
					};
					vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

					vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MainPipeline->m_Pipeline);

					vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MainPipeline->m_PipelineLayout,
						kGlobalSetIndex, 1, &instance->GlobalSet, 0, nullptr);

					// View Data set
					vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MainPipeline->m_PipelineLayout,
						kViewDataSetIndex, 1, &views.ViewDataSet, 0, nullptr);

					// Vertex & Index Buffers
					vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &drawBuffer->m_Buffer, (const VkDeviceSize*)&views.VertexBuffer.StartOffset);
					vkCmdBindIndexBuffer(cmdBuffer, drawBuffer->m_Buffer, views.IndexBuffer.StartOffset, sizeof(FUIIndex) == 4 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);

					for (SizeT i = 0; i < snapshot->renderPassArray.GetCount(); ++i)
					{
						const SizeT layerIndex = snapshot->renderPassArray[i].LayerIndex;
						const SizeT drawCmdStartIndex = snapshot->renderPassArray[i].DrawCmdStartIndex;
						const SizeT drawCmdCount = snapshot->renderPassArray[i].DrawCmdCount;

						// Draw Data set
						vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MainPipeline->m_PipelineLayout,
							3, 1, &views.DrawDataSets[layerIndex], 0, nullptr);

						// Layer Transform
						vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MainPipeline->m_PipelineLayout, 
							2, 1, &views.LayerTransformSets[layerIndex], 0, nullptr);

						for (SizeT drawCmdIndex = drawCmdStartIndex; drawCmdIndex < drawCmdStartIndex + drawCmdCount; drawCmdIndex++)
						{
							FUIDrawCmd drawCmd = snapshot->drawCmdArray[drawCmdIndex];

							u32 globalVertexOffset = snapshot->vertexSplits[layerIndex].StartOffset / sizeof(FUIVertex) + drawCmd.VertexOffset;
							u32 globalIndexOffset = snapshot->indexSplits[layerIndex].StartOffset / sizeof(FUIIndex) + drawCmd.IndexOffset;

							vkCmdDrawIndexed(cmdBuffer, drawCmd.IndexCount, 1, globalIndexOffset, (int32_t)globalVertexOffset, 0);
						}
					}
				}
				vkCmdEndRenderPass(cmdBuffer);

				numRenderPasses++;
			}
		}
		vkEndCommandBuffer(cmdBuffer);

		// - Submit -

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[m_FrameSlot];
		submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.Size();
		submitInfo.pWaitDstStageMask = waitSemaphoreStages.Data();
		submitInfo.pWaitSemaphores = waitSemaphores.Data();

		result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_RenderFinishedFences[m_FrameSlot]);
		VULKAN_CHECK(result, "Failed to submit Command Buffer.");

		// - Present -

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_FrameSlot];
		presentInfo.swapchainCount = (uint32_t)presentSwapChains.Size();
		presentInfo.pSwapchains = presentSwapChains.Data();
		presentInfo.pImageIndices = presentImageIndices.Data();

		result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			UpdateAllSwapChains();
		}

		FUSION_ASSERT(result == VK_SUCCESS || result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR,
			"Failed to Present SwapChains.");

		m_FrameSlot = (m_FrameSlot + 1) % kImageCount;

		m_PendingAtlasUploads.Clear();
	}

	// ------------------------------------------------------------------------------------------------------------
	// - Vulkan
	// ------------------------------------------------------------------------------------------------------------

	void FVulkanRenderBackend::InitializeVulkan()
	{
		m_PlatformBackend->RegisterEventSink(this);

		// - Instance -

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Fusion Application";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Fusion Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		appInfo.pNext = nullptr;

		TArray<const char*> requiredExtensions = FVulkanPlatform::GetRequiredVulkanInstanceExtensions();
		TArray<const char*> requiredLayers = FVulkanPlatform::GetRequiredInstanceLayers();

		VkInstanceCreateInfo instanceCI{};
		instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCI.flags = FVulkanPlatform::GetRequiredInstanceFlags();
		instanceCI.pNext = nullptr;

		instanceCI.pApplicationInfo = &appInfo;
		
		instanceCI.enabledLayerCount = (uint32_t)requiredLayers.Size();
		instanceCI.ppEnabledLayerNames = requiredLayers.Data();

		instanceCI.enabledExtensionCount = (uint32_t)requiredExtensions.Size();
		instanceCI.ppEnabledExtensionNames = requiredExtensions.Data();

		VkDebugUtilsMessengerCreateInfoEXT debugCI{};
		debugCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCI.pNext = nullptr;
		debugCI.flags = 0;

		if (FVulkanPlatform::IsValidationEnabled())
		{
			PopulateDebugMessengerCreateInfo(debugCI);
			instanceCI.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCI;
		}

		auto result = vkCreateInstance(&instanceCI, VULKAN_CPU_ALLOCATOR, &m_VulkanInstance);
		VULKAN_CHECK(result, "Failed to create Vulkan instance.");

		if (FVulkanPlatform::IsValidationEnabled())
		{
			result = CreateDebugUtilsMessengerEXT(m_VulkanInstance, &debugCI, VULKAN_CPU_ALLOCATOR, &m_VkMessenger);
			VULKAN_CHECK(result, "Failed to create Vulkan debug messenger.");
		}


		// - Physical Device -

		// Fetch all available physical devices
		u32 physicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDeviceCount, nullptr);
		TArray<VkPhysicalDevice> physicalDevices{ physicalDeviceCount };
		vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDeviceCount, physicalDevices.Data());

		if (physicalDeviceCount == 0)
		{
			FUSION_LOG_ERROR("Vulkan", "Failed to find any Vulkan-compatible physical devices.");
		}

#if FUSION_PLATFORM_MAC
		m_PhysicalDevice = physicalDevices[0];
#else
		if (physicalDeviceCount == 1)
		{
			m_PhysicalDevice = physicalDevices[0];
		}
		else
		{
			for (int i = 0; i < physicalDevices.Size(); ++i)
			{
				VkPhysicalDeviceProperties properties{};
				vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);

				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					m_PhysicalDevice = physicalDevices[i];
					break;
				}
			}
		}
#endif

		FUSION_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find a suitable Vulkan physical device.");

		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_PhysicalDeviceFeatures);
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_PhysicalDeviceMemoryProperties);

		m_IsUnifiedMemory = true;
		m_IsResizableBAR = false;

		for (int i = 0; i < (int)m_PhysicalDeviceMemoryProperties.memoryHeapCount; i++)
		{
			if (m_PhysicalDeviceMemoryProperties.memoryHeaps[i].flags != VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			{
				m_IsUnifiedMemory = false;
			}
		}


		// - Surface -

		FVulkanTempSurface tempSurfaceData = FVulkanPlatform::CreateTempSurface(m_VulkanInstance);
		{
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, tempSurfaceData.tempSurface, &m_SurfaceCapabilities);

			uint32_t surfaceFormatCount = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, tempSurfaceData.tempSurface, &surfaceFormatCount, nullptr);

			if (surfaceFormatCount > 0)
			{
				m_SurfaceFormats.Resize(surfaceFormatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, tempSurfaceData.tempSurface, &surfaceFormatCount, m_SurfaceFormats.Data());
			}
			
			uint32_t presentModesCount = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, tempSurfaceData.tempSurface, &presentModesCount, nullptr);

			if (presentModesCount > 0)
			{
				m_PresentationModes.Resize(presentModesCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, tempSurfaceData.tempSurface, &presentModesCount, m_PresentationModes.Data());
			}

			FUSION_ASSERT(!m_SurfaceFormats.Empty(), "Failed to find any Surface Formats.");
			FUSION_ASSERT(!m_PresentationModes.Empty(), "Failed to find any Surface Present Modes.");

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
			FUSION_ASSERT(queueFamilyCount > 0, "Failed to fetch Queue Family Properties");

			m_QueueFamilyProperties.Resize(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, m_QueueFamilyProperties.Data());

			m_QueueFamilyIndex = -1;

			for (uint32_t i = 0; i < queueFamilyCount; i++)
			{
				VkBool32 presentationSupported = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, tempSurfaceData.tempSurface, &presentationSupported);

				if ((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentationSupported)
				{
					m_QueueFamilyIndex = i;
					m_QueueCount = FMath::Min<uint32_t>(m_QueueFamilyProperties[i].queueCount, 2);
					break;
				}
			}

			FUSION_ASSERT(m_QueueFamilyIndex >= 0, "Failed to find a valid Queue Family.");
			FUSION_ASSERT(m_QueueCount > 0, "Failed to find a valid Queue Count.");
		}
		FVulkanPlatform::DestroyTempSurface(m_VulkanInstance, tempSurfaceData);


		// - Queues -

		float queuePriorities[2] = { 1.0f, 1.0f };

		VkDeviceQueueCreateInfo queueCI{};
		queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCI.queueCount = m_QueueCount;
		queueCI.pQueuePriorities = queuePriorities;
		queueCI.queueFamilyIndex = m_QueueFamilyIndex;


		// - Device -

		TArray<const char*> deviceExtensionNames{};

		uint32_t deviceExtensionCount = 0;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionCount, nullptr);
		TArray<VkExtensionProperties> deviceExtensionProperties(deviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionCount, deviceExtensionProperties.Data());

		deviceExtensionNames.Add(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		bool nonSemanticInfoExtFound = false;

		for (uint32_t i = 0; i < deviceExtensionCount; ++i)
		{
			const char* extName = deviceExtensionProperties[i].extensionName;

			// Required rule by Vulkan Specs, especially on Apple platform.
			if (strcmp(extName, "VK_KHR_portability_subset") == 0)
			{
				deviceExtensionNames.Add(extName);
			}

			// Needed for SPIR-V shader debug info (SPV_KHR_non_semantic_info).
			// Only present when the driver supports it; not required for release builds.
			if (strcmp(extName, VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME) == 0)
			{
				nonSemanticInfoExtFound = true;
				deviceExtensionNames.Add(extName);
			}
		}

#if FUSION_SHADER_DEBUG_SYMBOLS
		FUSION_ASSERT(nonSemanticInfoExtFound, "Shader debug symbols were enabled when the Vulkan Device does not support VK_KHR_shader_non_semantic_info extension!");
#endif

		VkDeviceCreateInfo deviceCI{};
		deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		
		deviceCI.queueCreateInfoCount = 1;
		deviceCI.pQueueCreateInfos = &queueCI;

		deviceCI.enabledExtensionCount = (uint32_t)deviceExtensionNames.Size();
		deviceCI.ppEnabledExtensionNames = deviceExtensionNames.Empty() ? nullptr : deviceExtensionNames.Data();

		VkPhysicalDeviceFeatures deviceFeaturesToUse{};
		deviceFeaturesToUse.samplerAnisotropy = VK_TRUE;

		if (m_PhysicalDeviceFeatures.sparseBinding && m_PhysicalDeviceFeatures.sparseResidencyImage2D)
		{
			deviceFeaturesToUse.sparseBinding = VK_TRUE;
			deviceFeaturesToUse.sparseResidencyImage2D = VK_TRUE;
		}

		deviceCI.pEnabledFeatures = &deviceFeaturesToUse;
		
		result = vkCreateDevice(m_PhysicalDevice, &deviceCI, VULKAN_CPU_ALLOCATOR, &m_Device);
		VULKAN_CHECK(result, "Failed to create VkDevice.");

		vkGetDeviceQueue(m_Device, m_QueueFamilyIndex, 0, &m_GraphicsQueue);

		if (m_QueueCount > 1)
		{
			vkGetDeviceQueue(m_Device, m_QueueFamilyIndex, 1, &m_PresentQueue);
		}
		else
		{
			m_PresentQueue = m_GraphicsQueue;
		}

		
		// - VMA Allocator -

		VmaAllocatorCreateInfo vmaAllocatorCI{};
		vmaAllocatorCI.instance = m_VulkanInstance;
		vmaAllocatorCI.device = m_Device;
		vmaAllocatorCI.physicalDevice = m_PhysicalDevice;
		vmaAllocatorCI.vulkanApiVersion = appInfo.apiVersion;

		result = vmaCreateAllocator(&vmaAllocatorCI, &m_Allocator);
		VULKAN_CHECK(result, "Failed to create VmaAllocator");

		// - Command Pool -

		VkCommandPoolCreateInfo commandPoolCI{};
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCI.queueFamilyIndex = m_QueueFamilyIndex;
		commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		result = vkCreateCommandPool(m_Device, &commandPoolCI, VULKAN_CPU_ALLOCATOR, &m_CommandPool);
		VULKAN_CHECK(result, "Failed to create VkCommandPool");


		// - Command Buffer -

		m_CommandBuffers.Resize(kImageCount);

		VkCommandBufferAllocateInfo commandBufferInfo{};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferInfo.commandBufferCount = (uint32_t)m_CommandBuffers.Size();
		commandBufferInfo.commandPool = m_CommandPool;
		commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		
		result = vkAllocateCommandBuffers(m_Device, &commandBufferInfo, m_CommandBuffers.Data());
		VULKAN_CHECK(result, "Failed to allocate VkCommandBuffer");


		// - Semaphores -

		VkSemaphoreCreateInfo semaphoreCI{};
		semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		
		m_RenderFinishedSemaphores.Resize(kImageCount);

		for (int i = 0; i < kImageCount; i++)
		{
			VkSemaphore semaphore = VK_NULL_HANDLE;

			result = vkCreateSemaphore(m_Device, &semaphoreCI, VULKAN_CPU_ALLOCATOR, &semaphore);
			VULKAN_CHECK(result, "Failed to create Render Finished semaphore.");

			m_RenderFinishedSemaphores[i] = semaphore;
		}


		// - Fences -

		VkFenceCreateInfo fenceCI{};
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		m_RenderFinishedFences.Resize(kImageCount);

		for (int i = 0; i < kImageCount; i++)
		{
			VkFence fence = VK_NULL_HANDLE;

			result = vkCreateFence(m_Device, &fenceCI, VULKAN_CPU_ALLOCATOR, &fence);
			VULKAN_CHECK(result, "Failed to create Render Finished fence.");

			m_RenderFinishedFences[i] = fence;
		}


		// - Render Pass -

		{
			VkRenderPassCreateInfo renderPassCI{};
			renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

			VkAttachmentDescription colorAttachment{};
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			VkAttachmentReference colorAttachmentRef{};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			renderPassCI.attachmentCount = 1;
			renderPassCI.pAttachments = &colorAttachment;

			VkSubpassDescription subpass{};
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			renderPassCI.subpassCount = 1;
			renderPassCI.pSubpasses = &subpass;

			result = vkCreateRenderPass(m_Device, &renderPassCI, VULKAN_CPU_ALLOCATOR, &m_RenderPass);
			VULKAN_CHECK(result, "Failed to create VkRenderPass.");
		}


		// - Main Graphics Pipeline -

		{
			const FShader* mainShader = Fusion::Shaders::FindShader("Fusion");
			FUSION_ASSERT(mainShader != nullptr, "Failed to find the main Fusion.slang shader!");

			const FShaderModule* vertexShader = mainShader->FindModule(FShaderStage::Vertex);
			const FShaderModule* fragmentShader = mainShader->FindModule(FShaderStage::Fragment);

			m_MainPipeline = new FGraphicsPipeline(m_Device);

			VkShaderModuleCreateInfo vertexModuleCI{};
			vertexModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			vertexModuleCI.codeSize = vertexShader->m_SPIRVSize;
			vertexModuleCI.pCode = (const uint32_t*)vertexShader->m_SPIRVData;
			
			result = vkCreateShaderModule(m_Device, &vertexModuleCI, VULKAN_CPU_ALLOCATOR, &m_MainPipeline->m_VertexModule);
			VULKAN_CHECK(result, "Failed to load Vertex Shader.");

			VkShaderModuleCreateInfo fragmentModuleCI{};
			fragmentModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			fragmentModuleCI.codeSize = fragmentShader->m_SPIRVSize;
			fragmentModuleCI.pCode = (const uint32_t*)fragmentShader->m_SPIRVData;

			result = vkCreateShaderModule(m_Device, &fragmentModuleCI, VULKAN_CPU_ALLOCATOR, &m_MainPipeline->m_FragmentModule);
			VULKAN_CHECK(result, "Failed to load Fragment Shader.");

			VkGraphicsPipelineCreateInfo graphicsPipelineCI{};
			graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

			VkPipelineShaderStageCreateInfo stages[2] = {};

			VkPipelineShaderStageCreateInfo& vertexStageCI = stages[0];
			vertexStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertexStageCI.flags = 0;
			vertexStageCI.pName = "main";
			vertexStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertexStageCI.module = m_MainPipeline->m_VertexModule;

			VkPipelineShaderStageCreateInfo& fragmentStageCI = stages[1];
			fragmentStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragmentStageCI.flags = 0;
			fragmentStageCI.pName = "main";
			fragmentStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentStageCI.module = m_MainPipeline->m_FragmentModule;

			graphicsPipelineCI.stageCount = 2;
			graphicsPipelineCI.pStages = &stages[0];

			VkPipelineVertexInputStateCreateInfo vertexInputState{};
			vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			
			VkVertexInputBindingDescription vertexInputBinding{};
			vertexInputBinding.binding = 0;
			vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexInputBinding.stride = sizeof(FUIVertex);

			vertexInputState.vertexBindingDescriptionCount = 1;
			vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
			
			std::array<VkVertexInputAttributeDescription, 4> vertexAttributes{};
			vertexAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
			vertexAttributes[0].binding = 0;
			vertexAttributes[0].location = 0;
			vertexAttributes[0].offset = offsetof(FUIVertex, pos);

			vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
			vertexAttributes[1].binding = 0;
			vertexAttributes[1].location = 1;
			vertexAttributes[1].offset = offsetof(FUIVertex, uv);

			vertexAttributes[2].format = VK_FORMAT_R8G8B8A8_UNORM;
			vertexAttributes[2].binding = 0;
			vertexAttributes[2].location = 2;
			vertexAttributes[2].offset = offsetof(FUIVertex, color);

			vertexAttributes[3].format = VK_FORMAT_R32_UINT;
			vertexAttributes[3].binding = 0;
			vertexAttributes[3].location = 3;
			vertexAttributes[3].offset = offsetof(FUIVertex, drawItemIndex);

			vertexInputState.vertexAttributeDescriptionCount = vertexAttributes.size();
			vertexInputState.pVertexAttributeDescriptions = vertexAttributes.data();

			graphicsPipelineCI.pVertexInputState = &vertexInputState;

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
			inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			
			graphicsPipelineCI.pInputAssemblyState = &inputAssemblyState;

			graphicsPipelineCI.renderPass = m_RenderPass;
			graphicsPipelineCI.subpass = 0;

			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;

			graphicsPipelineCI.pViewportState = &viewportState;

			std::array dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
			dynamicState.pDynamicStates = dynamicStates.data();

			graphicsPipelineCI.pDynamicState = &dynamicState;

			VkPipelineLayoutCreateInfo pipelineLayoutCI{};
			pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCI.pushConstantRangeCount = 0;

			// Set 0
			{
				VkDescriptorSetLayoutCreateInfo setLayoutCI{};
				setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				
				TArray<VkDescriptorSetLayoutBinding> bindings{};

				bindings.Add({ // _TextureArray
					.binding = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = nullptr
				});

				bindings.Add({ // _FontAtlas
					.binding = 2,
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = nullptr
				});

				VkSamplerCreateInfo fontSamplerCI{};
				fontSamplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				fontSamplerCI.addressModeU = fontSamplerCI.addressModeV = fontSamplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				fontSamplerCI.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
				fontSamplerCI.anisotropyEnable = VK_TRUE;
				fontSamplerCI.maxAnisotropy = 16;
				fontSamplerCI.compareEnable = VK_FALSE;
				fontSamplerCI.minLod = 0;
				fontSamplerCI.minFilter = VK_FILTER_LINEAR;
				fontSamplerCI.magFilter = VK_FILTER_LINEAR;
				fontSamplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				fontSamplerCI.unnormalizedCoordinates = VK_FALSE;

				VkSampler sampler = VK_NULL_HANDLE;
				result = vkCreateSampler(m_Device, &fontSamplerCI, VULKAN_CPU_ALLOCATOR, &sampler);
				VULKAN_CHECK(result, "Failed to create sampler.");

				bindings.Add({ // _FontAtlasSampler
					.binding = 3,
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = &sampler
				});

				m_MainPipeline->m_ImmutableSamplers.Add(sampler);

				setLayoutCI.bindingCount = bindings.Size();
				setLayoutCI.pBindings = bindings.Data();

				VkDescriptorSetLayout setLayout = nullptr;
				result = vkCreateDescriptorSetLayout(m_Device, &setLayoutCI, VULKAN_CPU_ALLOCATOR, &setLayout);
				VULKAN_CHECK(result, "Failed to create Set Layout.");
				
				m_MainPipeline->m_SetLayouts.Add(setLayout);
			}
			
			// Set 1
			{
				VkDescriptorSetLayoutCreateInfo setLayoutCI{};
				setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				
				TArray<VkDescriptorSetLayoutBinding> bindings{};
				bindings.Add({ // _ViewData
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
					.pImmutableSamplers = nullptr
				});

				setLayoutCI.bindingCount = (uint32_t)bindings.Size();
				setLayoutCI.pBindings = bindings.Data();
				
				VkDescriptorSetLayout setLayout = nullptr;
				result = vkCreateDescriptorSetLayout(m_Device, &setLayoutCI, VULKAN_CPU_ALLOCATOR, &setLayout);
				VULKAN_CHECK(result, "Failed to create Set Layout.");

				m_MainPipeline->m_SetLayouts.Add(setLayout);
			}

			// Set 2
			{
				VkDescriptorSetLayoutCreateInfo setLayoutCI{};
				setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

				TArray<VkDescriptorSetLayoutBinding> bindings{};
				bindings.Add({ // _LayerTransform
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
					.pImmutableSamplers = nullptr
				});

				setLayoutCI.bindingCount = (uint32_t)bindings.Size();
				setLayoutCI.pBindings = bindings.Data();

				VkDescriptorSetLayout setLayout = nullptr;
				result = vkCreateDescriptorSetLayout(m_Device, &setLayoutCI, VULKAN_CPU_ALLOCATOR, &setLayout);
				VULKAN_CHECK(result, "Failed to create Set Layout.");

				m_MainPipeline->m_SetLayouts.Add(setLayout);
			}

			// Set 3
			{
				VkDescriptorSetLayoutCreateInfo setLayoutCI{};
				setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

				TArray<VkDescriptorSetLayoutBinding> bindings{};

				bindings.Add({ // _DrawItems
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = nullptr
				});

				bindings.Add({ // _ClipRects
					.binding = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = nullptr
				});

				bindings.Add({ // _GradientStops
					.binding = 2,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = nullptr
				});

				setLayoutCI.bindingCount = (uint32_t)bindings.Size();
				setLayoutCI.pBindings = bindings.Data();

				VkDescriptorSetLayout setLayout = nullptr;
				result = vkCreateDescriptorSetLayout(m_Device, &setLayoutCI, VULKAN_CPU_ALLOCATOR, &setLayout);
				VULKAN_CHECK(result, "Failed to create Set Layout.");

				m_MainPipeline->m_SetLayouts.Add(setLayout);
			}

			pipelineLayoutCI.setLayoutCount = (uint32_t)m_MainPipeline->m_SetLayouts.Size();
			pipelineLayoutCI.pSetLayouts = m_MainPipeline->m_SetLayouts.Data();

			result = vkCreatePipelineLayout(m_Device, &pipelineLayoutCI, VULKAN_CPU_ALLOCATOR, &m_MainPipeline->m_PipelineLayout);
			VULKAN_CHECK(result, "Failed to create Main Pipeline Layout.");

			graphicsPipelineCI.layout = m_MainPipeline->m_PipelineLayout;

			VkPipelineColorBlendStateCreateInfo colorBlendState{};
			colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			
			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.blendEnable = VK_TRUE;
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

			colorBlendState.attachmentCount = 1;
			colorBlendState.pAttachments = &colorBlendAttachment;
			
			colorBlendState.logicOpEnable = VK_FALSE;
			
			graphicsPipelineCI.pColorBlendState = &colorBlendState;

			VkPipelineRasterizationStateCreateInfo rasterizationState{};
			rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationState.cullMode = VK_CULL_MODE_NONE;
			rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterizationState.depthBiasEnable = VK_FALSE;
			rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizationState.lineWidth = 1.0f;
			
			graphicsPipelineCI.pRasterizationState = &rasterizationState;

			VkPipelineMultisampleStateCreateInfo multisampleState{};
			multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			graphicsPipelineCI.pMultisampleState = &multisampleState;

			VkPipelineDepthStencilStateCreateInfo depthStencilState{};
			depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilState.depthTestEnable = VK_FALSE;
			depthStencilState.depthWriteEnable = VK_FALSE;
			depthStencilState.stencilTestEnable = VK_FALSE;

			graphicsPipelineCI.pDepthStencilState = &depthStencilState;

			result = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineCI, VULKAN_CPU_ALLOCATOR, &m_MainPipeline->m_Pipeline);
			VULKAN_CHECK(result, "Failed to create Main Graphics Pipeline.");
		}

		// - Mip Map Graphics Pipeline -
		if (false) // Disabled for now
		{
			const FShader* mipMapShader = Fusion::Shaders::FindShader("MipMap");
			FUSION_ASSERT(mipMapShader != nullptr, "Failed to find the main MipMap.slang shader!");

			const FShaderModule* vertexShader = mipMapShader->FindModule(FShaderStage::Vertex);
			const FShaderModule* fragmentShader = mipMapShader->FindModule(FShaderStage::Fragment);

			m_MipMapPipeline = new FGraphicsPipeline(m_Device);

			VkShaderModuleCreateInfo vertexModuleCI{};
			vertexModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			vertexModuleCI.codeSize = vertexShader->m_SPIRVSize;
			vertexModuleCI.pCode = (const uint32_t*)vertexShader->m_SPIRVData;

			result = vkCreateShaderModule(m_Device, &vertexModuleCI, VULKAN_CPU_ALLOCATOR, &m_MipMapPipeline->m_VertexModule);
			VULKAN_CHECK(result, "Failed to load Vertex Shader.");

			VkShaderModuleCreateInfo fragmentModuleCI{};
			fragmentModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			fragmentModuleCI.codeSize = fragmentShader->m_SPIRVSize;
			fragmentModuleCI.pCode = (const uint32_t*)fragmentShader->m_SPIRVData;

			result = vkCreateShaderModule(m_Device, &fragmentModuleCI, VULKAN_CPU_ALLOCATOR, &m_MipMapPipeline->m_FragmentModule);
			VULKAN_CHECK(result, "Failed to load Fragment Shader.");

			VkGraphicsPipelineCreateInfo graphicsPipelineCI{};
			graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

			VkPipelineShaderStageCreateInfo stages[2] = {};

			VkPipelineShaderStageCreateInfo& vertexStageCI = stages[0];
			vertexStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertexStageCI.flags = 0;
			vertexStageCI.pName = "main";
			vertexStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertexStageCI.module = m_MipMapPipeline->m_VertexModule;

			VkPipelineShaderStageCreateInfo& fragmentStageCI = stages[1];
			fragmentStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragmentStageCI.flags = 0;
			fragmentStageCI.pName = "main";
			fragmentStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentStageCI.module = m_MipMapPipeline->m_FragmentModule;

			graphicsPipelineCI.stageCount = 2;
			graphicsPipelineCI.pStages = &stages[0];

			VkPipelineVertexInputStateCreateInfo vertexInputState{};
			vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			VkVertexInputBindingDescription vertexInputBinding{};
			vertexInputBinding.binding = 0;
			vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexInputBinding.stride = sizeof(FUIQuadVertex);

			vertexInputState.vertexBindingDescriptionCount = 1;
			vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;

			std::array<VkVertexInputAttributeDescription, 2> vertexAttributes{};
			vertexAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
			vertexAttributes[0].binding = 0;
			vertexAttributes[0].location = 0;
			vertexAttributes[0].offset = offsetof(FUIQuadVertex, pos);

			vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
			vertexAttributes[1].binding = 0;
			vertexAttributes[1].location = 1;
			vertexAttributes[1].offset = offsetof(FUIQuadVertex, uv);

			vertexInputState.vertexAttributeDescriptionCount = vertexAttributes.size();
			vertexInputState.pVertexAttributeDescriptions = vertexAttributes.data();

			graphicsPipelineCI.pVertexInputState = &vertexInputState;

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
			inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			graphicsPipelineCI.pInputAssemblyState = &inputAssemblyState;

			graphicsPipelineCI.renderPass = m_RenderPass;
			graphicsPipelineCI.subpass = 0;

			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;

			graphicsPipelineCI.pViewportState = &viewportState;

			std::array dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
			dynamicState.pDynamicStates = dynamicStates.data();

			graphicsPipelineCI.pDynamicState = &dynamicState;

			VkPipelineLayoutCreateInfo pipelineLayoutCI{};
			pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCI.pushConstantRangeCount = 0;

			// Set 0
			{
				VkDescriptorSetLayoutCreateInfo setLayoutCI{};
				setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

				TArray<VkDescriptorSetLayoutBinding> bindings{};

				bindings.Add({ // _InputTexture
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = nullptr
				});

				VkSamplerCreateInfo fontSamplerCI{};
				fontSamplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				fontSamplerCI.addressModeU = fontSamplerCI.addressModeV = fontSamplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				fontSamplerCI.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
				fontSamplerCI.anisotropyEnable = VK_TRUE;
				fontSamplerCI.maxAnisotropy = 16;
				fontSamplerCI.compareEnable = VK_FALSE;
				fontSamplerCI.minLod = 0;
				fontSamplerCI.minFilter = VK_FILTER_LINEAR;
				fontSamplerCI.magFilter = VK_FILTER_LINEAR;
				fontSamplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				fontSamplerCI.unnormalizedCoordinates = VK_FALSE;

				VkSampler sampler = VK_NULL_HANDLE;
				result = vkCreateSampler(m_Device, &fontSamplerCI, VULKAN_CPU_ALLOCATOR, &sampler);
				VULKAN_CHECK(result, "Failed to create sampler.");

				m_MipMapPipeline->m_ImmutableSamplers.Add(sampler);

				bindings.Add({ // _InputTextureSampler
					.binding = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = m_MipMapPipeline->m_ImmutableSamplers.Data()
				});

				setLayoutCI.bindingCount = bindings.Size();
				setLayoutCI.pBindings = bindings.Data();

				VkDescriptorSetLayout setLayout = nullptr;
				result = vkCreateDescriptorSetLayout(m_Device, &setLayoutCI, VULKAN_CPU_ALLOCATOR, &setLayout);
				VULKAN_CHECK(result, "Failed to create Set Layout.");

				m_MipMapPipeline->m_SetLayouts.Add(setLayout);
			}

			pipelineLayoutCI.setLayoutCount = (uint32_t)m_MipMapPipeline->m_SetLayouts.Size();
			pipelineLayoutCI.pSetLayouts = m_MipMapPipeline->m_SetLayouts.Data();

			result = vkCreatePipelineLayout(m_Device, &pipelineLayoutCI, VULKAN_CPU_ALLOCATOR, &m_MipMapPipeline->m_PipelineLayout);
			VULKAN_CHECK(result, "Failed to create Main Pipeline Layout.");
		}

		// - Descriptors -

		for (int i = 0; i < kImageCount; i++)
		{
			m_PoolsPerFrame.Add(new FDescriptorPool(this, m_Device));
		}

		// - UI Render Buffer -

		for (int i = 0; i < kImageCount; i++)
		{
			m_UIDrawDataBuffers[i] = new FMappedBuffer(this, kBufferInitialSize, kBufferGrowSize);
		}

		// - Staging Buffer -

		for (int i = 0; i < kImageCount; i++)
		{
			m_StagingBuffers[i] = new FMappedBuffer(this, kStagingBufferInitialSize, kStagingBufferGrowSize);
		}

		// - Null Buffer -

		m_NullBuffer = new FBuffer(this, 256);

		m_NullBufferInfo = {
			.buffer = m_NullBuffer->m_Buffer,
			.offset = 0,
			.range = m_NullBuffer->m_BufferSize
		};

		// - Transient Resources -

		m_UploadArena.Grow();
		m_BufferImageCopies.Grow();
	}

	void FVulkanRenderBackend::ShutdownVulkan()
	{
		vkDeviceWaitIdle(m_Device);

		m_BufferImageCopies.Free();
		m_UploadArena.Free();

		m_AtlasIndexAllocator = 0;
		m_AtlasesByHandle.Clear();

		m_RenderTargetIndexAllocator = 0;
		m_RenderTargetsByHandle.Clear();

		delete m_NullBuffer;

		for (SizeT i = 0; i < m_UIDrawDataBuffers.Size(); i++)
		{
			m_UIDrawDataBuffers[i]->DeferredDestroy();
			m_UIDrawDataBuffers[i] = nullptr;
		}

		for (SizeT i = 0; i < m_StagingBuffers.Size(); i++)
		{
			m_StagingBuffers[i]->DeferredDestroy();
			m_StagingBuffers[i] = nullptr;
		}

		for (SizeT i = 0; i < m_DeferredDestruction.Size(); i++)
		{
			m_DeferredDestruction[i].m_Destruction.Execute();
		}
		m_DeferredDestruction.Clear();

		for (SizeT i = 0; i < kImageCount; i++)
		{
			delete m_PoolsPerFrame[i];
		}
		m_PoolsPerFrame.Clear();

		m_SwapChainsByWindowHandle.Clear();

		m_PlatformBackend->DeregisterEventSink(this);

		if (m_RenderPass)
		{
			vkDestroyRenderPass(m_Device, m_RenderPass, VULKAN_CPU_ALLOCATOR);
			m_RenderPass = VK_NULL_HANDLE;
		}

		if (m_MipMapRenderPass)
		{
			vkDestroyRenderPass(m_Device, m_MipMapRenderPass, VULKAN_CPU_ALLOCATOR);
			m_MipMapRenderPass = VK_NULL_HANDLE;
		}

		m_MipMapPipeline = nullptr;
		m_MainPipeline = nullptr;

		for (int i = 0; i < m_RenderFinishedFences.Size(); i++)
		{
			vkDestroyFence(m_Device, m_RenderFinishedFences[i], VULKAN_CPU_ALLOCATOR);
		}
		m_RenderFinishedFences.Clear();

		for (int i = 0; i < m_RenderFinishedSemaphores.Size(); i++)
		{
			vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], VULKAN_CPU_ALLOCATOR);
		}
		m_RenderFinishedSemaphores.Clear();

		m_CommandBuffers.Clear();

		if (m_CommandPool)
		{
			vkDestroyCommandPool(m_Device, m_CommandPool, VULKAN_CPU_ALLOCATOR);
			m_CommandPool = VK_NULL_HANDLE;
		}

		if (m_Allocator)
		{
			vmaDestroyAllocator(m_Allocator);
			m_Allocator = VK_NULL_HANDLE;
		}

		for (int i = 0; i < m_DeferredDestruction.Size(); i++)
		{
			m_DeferredDestruction[i].m_Destruction.ExecuteIfBound();
		}
		m_DeferredDestruction.Clear();

		if (m_Device)
		{
			vkDestroyDevice(m_Device, VULKAN_CPU_ALLOCATOR);
			m_Device = VK_NULL_HANDLE;
		}

		if (m_VkMessenger != VK_NULL_HANDLE)
		{
			DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_VkMessenger, VULKAN_CPU_ALLOCATOR);
		}
		m_VkMessenger = VK_NULL_HANDLE;

		vkDestroyInstance(m_VulkanInstance, VULKAN_CPU_ALLOCATOR);
		m_VulkanInstance = VK_NULL_HANDLE;
	}

	void FVulkanRenderBackend::TransitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout curLayout, VkImageLayout toLayout, VkImageSubresourceRange subresourceRange)
	{
		if (curLayout == toLayout)
			return;

		VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkDependencyFlags dependencyFlags = 0;

		VkImageMemoryBarrier imageBarrier{};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		switch (curLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			imageBarrier.srcAccessMask = 0;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			imageBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			imageBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			srcStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		}

		switch (toLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		}
		
		imageBarrier.oldLayout = curLayout;
		imageBarrier.newLayout = toLayout;
		imageBarrier.image = image;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.subresourceRange = subresourceRange;

		vkCmdPipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, dependencyFlags, 
			0, nullptr, 
			0, nullptr, 
			1, &imageBarrier);
	}

	void FVulkanRenderBackend::UpdateAllSwapChains()
	{
		for (auto [windowHandle, swapChain] : m_SwapChainsByWindowHandle)
		{
			CreateOrUpdateSwapChain(windowHandle);
		}
	}

	void FVulkanRenderBackend::CreateOrUpdateSwapChain(FWindowHandle window)
	{
		IPtr<FSwapChain> swapChain = m_SwapChainsByWindowHandle[window];

		VkResult result = VK_SUCCESS;

		if (swapChain == nullptr)
		{
			swapChain = new FSwapChain(this, m_Device);

			swapChain->m_Surface = FVulkanPlatform::CreateSurface(this, window);

			swapChain->m_ImageAcquiredSemaphores.Resize(kImageCount);

			for (int i = 0; i < kImageCount; i++)
			{
				VkSemaphoreCreateInfo semaphoreCI{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0
				};

				result = vkCreateSemaphore(m_Device, &semaphoreCI, VULKAN_CPU_ALLOCATOR, &swapChain->m_ImageAcquiredSemaphores[i]);
				VULKAN_CHECK(result, "Failed to create Image Acquired semaphore.");
			}

			m_SwapChainsByWindowHandle[window] = swapChain;
		}

		swapChain->m_Images.Clear();

		swapChain->m_Window = window;

		VkSwapchainKHR oldSwapChain = swapChain->m_SwapChain;

		VkSurfaceCapabilitiesKHR surfaceCapabilities{};

		result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, swapChain->m_Surface, &surfaceCapabilities);
		VULKAN_CHECK(result, "Failed to fetch surface capabilities");

		VkSwapchainCreateInfoKHR swapChainCI{};
		swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

		swapChainCI.oldSwapchain = oldSwapChain;
		
		VkExtent2D extent{};

		if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			extent = surfaceCapabilities.currentExtent;
		}
		else
		{
			FVec2i windowSize = m_PlatformBackend->GetWindowSizeInPixels(window);
			if (windowSize.x == 0 || windowSize.y == 0)
			{
				return;
			}

			extent.width = windowSize.width;
			extent.height = windowSize.height;

			// Surface also defines max & min values. So make sure the values are clamped
			extent.width = FMath::Clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
			extent.height = FMath::Clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
		}

		swapChainCI.imageExtent = extent;

		swapChain->m_Width = extent.width;
		swapChain->m_Height = extent.height;

		uint32_t presentModesCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, swapChain->m_Surface, &presentModesCount, nullptr);

		TArray<VkPresentModeKHR> presentModes(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, swapChain->m_Surface, &presentModesCount, presentModes.Data());

		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		//presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

		swapChainCI.presentMode = presentMode;
		swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		swapChainCI.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
		swapChainCI.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapChainCI.imageArrayLayers = 1;

		swapChainCI.minImageCount = surfaceCapabilities.minImageCount;
		swapChainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapChainCI.clipped = VK_TRUE;

		swapChainCI.surface = swapChain->m_Surface;

		result = vkCreateSwapchainKHR(m_Device, &swapChainCI, VULKAN_CPU_ALLOCATOR, &swapChain->m_SwapChain);
		VULKAN_CHECK(result, "Failed to create Vulkan SwapChain.");

		uint32_t swapChainImageCount = 0;
		vkGetSwapchainImagesKHR(m_Device, swapChain->m_SwapChain, &swapChainImageCount, nullptr);

		TArray<VkImage> swapChainImages(swapChainImageCount);
		vkGetSwapchainImagesKHR(m_Device, swapChain->m_SwapChain, &swapChainImageCount, swapChainImages.Data());

		for (uint32_t i = 0; i < swapChainImageCount; i++)
		{
			IPtr<FTexture> texture = new FTexture(this, m_Device);

			texture->m_IsExternalImage = true;
			texture->m_Image = swapChainImages[i];
			texture->m_Format = swapChainCI.imageFormat;

			VkImageViewCreateInfo imageViewCI{};
			imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			imageViewCI.format = swapChainCI.imageFormat;

			imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCI.subresourceRange.baseArrayLayer = 0;
			imageViewCI.subresourceRange.layerCount = 1;
			imageViewCI.subresourceRange.baseMipLevel = 0;
			imageViewCI.subresourceRange.levelCount = 1;

			imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCI.image = texture->m_Image;
			
			result = vkCreateImageView(m_Device, &imageViewCI, VULKAN_CPU_ALLOCATOR, &texture->m_ImageView);
			VULKAN_CHECK(result, "Failed to create Vulkan ImageView for SwapChain.");

			swapChain->m_Images.Add(texture);
		}

		if (!swapChain->m_FrameBuffers.Empty())
		{
			for (int i = 0; i < swapChain->m_FrameBuffers.Size(); i++)
			{
				VkFramebuffer oldFrameBuffer = swapChain->m_FrameBuffers[i];

				if (oldFrameBuffer)
				{
					DeferDestruction([this, oldFrameBuffer]
					{
						vkDestroyFramebuffer(m_Device, oldFrameBuffer, VULKAN_CPU_ALLOCATOR);
					});
				}
			}

			swapChain->m_FrameBuffers.Clear();
		}

		swapChain->m_FrameBuffers.Resize(swapChain->m_Images.Size());

		for (int i = 0; i < swapChain->m_FrameBuffers.Size(); i++)
		{
			VkFramebufferCreateInfo frameBufferCI{};
			frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frameBufferCI.attachmentCount = 1;
			frameBufferCI.pAttachments = &swapChain->m_Images[i]->m_ImageView;
			frameBufferCI.width = swapChain->m_Width;
			frameBufferCI.height = swapChain->m_Height;
			frameBufferCI.renderPass = m_RenderPass;
			frameBufferCI.layers = 1;

			result = vkCreateFramebuffer(m_Device, &frameBufferCI, VULKAN_CPU_ALLOCATOR, &swapChain->m_FrameBuffers[i]);
			VULKAN_CHECK(result, "Failed to create FrameBuffer.");
		}

		VkFramebufferCreateInfo frameBufferCI{};
		frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCI.renderPass = m_RenderPass;
		frameBufferCI.attachmentCount = 1;

		if (oldSwapChain)
		{
			DeferDestruction([this, oldSwapChain]
			{
				vkDestroySwapchainKHR(m_Device, oldSwapChain, VULKAN_CPU_ALLOCATOR);
			});
		}
	}

	void FVulkanRenderBackend::DestroySwapChain(FWindowHandle window)
	{
		if (!m_SwapChainsByWindowHandle.KeyExists(window))
		{
			return;
		}

		IPtr<FSwapChain> swapChain = m_SwapChainsByWindowHandle[window];

		if (swapChain == nullptr)
		{
			return;
		}

		swapChain = nullptr;
		m_SwapChainsByWindowHandle.Remove(window);
	}

	void FVulkanRenderBackend::OnWindowCreated(FWindowHandle window)
	{
		CreateOrUpdateSwapChain(window);
	}

	void FVulkanRenderBackend::OnWindowDestroyed(FWindowHandle window)
	{
		DestroySwapChain(window);
	}

	void FVulkanRenderBackend::OnWindowResized(FWindowHandle window, [[maybe_unused]] u32 newWidth, [[maybe_unused]] u32 newHeight)
	{
		CreateOrUpdateSwapChain(window);
	}

	void FVulkanRenderBackend::OnWindowMaximized(FWindowHandle window)
	{
		CreateOrUpdateSwapChain(window);
	}

	void FVulkanRenderBackend::OnWindowMinimized(FWindowHandle window)
	{
		CreateOrUpdateSwapChain(window);
	}

	void FVulkanRenderBackend::OnWindowRestored(FWindowHandle window)
	{
		CreateOrUpdateSwapChain(window);
	}

}

#include "VulkanRHIPrivate.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#include "PAL/VulkanPlatform.h"

#define VULKAN_ASSERT(vkResult, message) FUSION_ASSERT(vkResult == VK_SUCCESS, message)

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
	// Descriptor Set
	// -----------------------------------------------------------------

	FDescriptorSet::~FDescriptorSet()
	{
		if (!m_RenderBackend->IsDescriptorPoolAlive())
			return;

		VkDevice device = m_Device;
		VkDescriptorPool pool = m_OwningPool;
		VkDescriptorSet set = m_Set;

		m_RenderBackend->DeferDestruction([device, pool, set]
		{
			vkFreeDescriptorSets(device, pool, 1, &set);
		});
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
		FArray<VkDescriptorPoolSize, 10> poolSizes = {
			{.type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 32 },
			{.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = 16 },
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
		poolCI.poolSizeCount = poolSizes.Size();
		poolCI.pPoolSizes = poolSizes.Data();
		poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VkDescriptorPool pool = VK_NULL_HANDLE;

		auto result = vkCreateDescriptorPool(m_Device, &poolCI, VULKAN_CPU_ALLOCATOR, &pool);
		VULKAN_ASSERT(result, "Failed to create VkDescriptorPool");

		m_Pools.Add(pool);
	}

	FDescriptorSet* FDescriptorPool::Allocate(VkDescriptorSetLayout setLayout)
	{
		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = m_Pools.Last();
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &setLayout;

		VkDescriptorSet set = VK_NULL_HANDLE;

		auto result = vkAllocateDescriptorSets(m_Device, &allocateInfo, &set);
		VULKAN_ASSERT(result, "Failed to allocate VkDescriptorSet");

		return new FDescriptorSet(m_RenderBackend, m_Device, allocateInfo.descriptorPool);
	}

	// -----------------------------------------------------------------
	// Texture
	// -----------------------------------------------------------------

	FTexture::~FTexture()
	{
		VkImage image = !m_IsExternalImage ? m_Image : VK_NULL_HANDLE;
		VkImageView imageView = m_ImageView;
		VkDevice device = m_Device;

		if (imageView || image)
		{
			m_RenderBackend->DeferDestruction([image, imageView, device]
			{
				if (imageView)
					vkDestroyImageView(device, imageView, VULKAN_CPU_ALLOCATOR);
				if (image)
					vkDestroyImage(device, image, VULKAN_CPU_ALLOCATOR);
			});
		}

		m_IsExternalImage = false;
		m_Image = VK_NULL_HANDLE;
		m_ImageView = VK_NULL_HANDLE;
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

		vkDestroyShaderModule(m_Device, m_VertexModule, VULKAN_CPU_ALLOCATOR);
		vkDestroyShaderModule(m_Device, m_FragmentModule, VULKAN_CPU_ALLOCATOR);
	}


	// -----------------------------------------------------------------
	// Vulkan Render Backend
	// -----------------------------------------------------------------

	FRenderCapabilities FVulkanRenderBackend::GetRenderCapabilities()
	{
		// TODO
		return {

		};
	}

	bool FVulkanRenderBackend::InitializeInstance(FInstanceHandle instance)
	{
		if (instances.KeyExists(instance))
		{
			return true;
		}

		if (m_VulkanInstance == VK_NULL_HANDLE)
		{
			InitializeVulkan();
		}

		IntrusivePtr<FRenderInstance> renderInstance = new FRenderInstance();
		


		instances.Add(instance, renderInstance);

		return true;
	}

	void FVulkanRenderBackend::ShutdownInstance(FInstanceHandle instance)
	{
		if (!instances.KeyExists(instance))
		{
			return;
		}

		instances.Remove(instance);

		if (instances.IsEmpty())
		{
			ShutdownVulkan();
		}
	}

	void FVulkanRenderBackend::RenderTick()
	{
		if (m_SwapChainsByWindowHandle.IsEmpty())
			return;

		VkResult result = VK_SUCCESS;

		constexpr uint64_t kSwapChainTimeOut = UINT64_MAX;
		constexpr uint64_t kFenceTimeOut = UINT64_MAX;

		FArray<VkSemaphore> waitSemaphores;
		FArray<VkPipelineStageFlags> waitSemaphoreStages;

		FArray<VkSwapchainKHR> presentSwapChains{};
		FArray<uint32_t> presentImageIndices{};

		result = vkWaitForFences(m_Device, 1, &m_RenderFinishedFences[m_FrameSlot], VK_TRUE, kFenceTimeOut);
		VULKAN_ASSERT(result, "Failed to wait on Render Finished Fence.");

		vkResetFences(m_Device, 1, &m_RenderFinishedFences[m_FrameSlot]);

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

			VULKAN_ASSERT(result, "Failed to acquire image from SwapChain.");

			waitSemaphores.Add(swapChain->m_ImageAcquiredSemaphores[m_FrameSlot]);
			waitSemaphoreStages.Add(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

			presentSwapChains.Add(swapChain->m_SwapChain);
			presentImageIndices.Add(swapChain->m_CurrentImageIndex);
		}

		TickDestructionQueue();

		VkCommandBuffer cmdBuffer = m_CommandBuffers[m_FrameSlot];

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		int numRenderPasses = 0;
		
		vkBeginCommandBuffer(cmdBuffer, &beginInfo);
		{
			for (auto [windowHandle, swapChain] : m_SwapChainsByWindowHandle)
			{
				VkClearValue colorClear;
				colorClear.color = { { 0.0f, 0.0f, 0.3f, 1.0f } }; // Opaque black

				VkRenderPassBeginInfo renderPassInfo{};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassInfo.clearValueCount = 1;
				renderPassInfo.pClearValues = &colorClear;
				renderPassInfo.framebuffer = swapChain->m_FrameBuffers[swapChain->m_CurrentImageIndex];
				renderPassInfo.renderArea = {
					.offset = { .x = 0, .y = 0 },
					.extent = { .width = swapChain->width, .height = swapChain->height }
				};
				renderPassInfo.renderPass = m_RenderPass;
				
				vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
				{
					
				}
				vkCmdEndRenderPass(cmdBuffer);

				numRenderPasses++;
			}
		}
		vkEndCommandBuffer(cmdBuffer);

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
		VULKAN_ASSERT(result, "Failed to submit Command Buffer.");

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

		m_FrameSlot = (m_FrameSlot + 1) % ImageCount;
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
	// - Vulkan
	// ------------------------------------------------------------------------------------------------------------

	void FVulkanRenderBackend::InitializeVulkan()
	{
		m_PlatformBackend->SetRenderBackendEventSink(this);

		// - Instance -

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Fusion Application";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Fusion Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		appInfo.pNext = nullptr;

		FArray<const char*> requiredExtensions = FVulkanPlatform::GetRequiredVulkanInstanceExtensions();
		FArray<const char*> requiredLayers = FVulkanPlatform::GetRequiredInstanceLayers();

		VkInstanceCreateInfo instanceCI{};
		instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCI.flags = 0;
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
		VULKAN_ASSERT(result, "Failed to create Vulkan instance.");

		if (FVulkanPlatform::IsValidationEnabled())
		{
			result = CreateDebugUtilsMessengerEXT(m_VulkanInstance, &debugCI, VULKAN_CPU_ALLOCATOR, &m_VkMessenger);
			VULKAN_ASSERT(result, "Failed to create Vulkan debug messenger.");
		}


		// - Physical Device -

		// Fetch all available physical devices
		u32 physicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDeviceCount, nullptr);
		FArray<VkPhysicalDevice> physicalDevices{ physicalDeviceCount };
		vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDeviceCount, physicalDevices.Data());

		if (physicalDeviceCount == 0)
		{
			FUSION_LOG_ERROR("Vulkan", "Failed to find any Vulkan-compatible physical devices.");
		}

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

		FUSION_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find a suitable Vulkan physical device.");

		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);

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

		FArray<const char*> deviceExtensionNames{};

		uint32_t deviceExtensionCount = 0;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionCount, nullptr);
		FArray<VkExtensionProperties> deviceExtensionProperties(deviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionCount, deviceExtensionProperties.Data());

		deviceExtensionNames.Add(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		for (uint32_t i = 0; i < deviceExtensionCount; ++i)
		{
			// Required rule by Vulkan Specs, especially on Apple platform.
			if (strcmp(deviceExtensionProperties[i].extensionName, "VK_KHR_portability_subset") == 0)
			{
				deviceExtensionNames.Add(deviceExtensionProperties[i].extensionName);
			}
		}

		VkDeviceCreateInfo deviceCI{};
		deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		
		deviceCI.queueCreateInfoCount = 1;
		deviceCI.pQueueCreateInfos = &queueCI;

		deviceCI.enabledExtensionCount = (uint32_t)deviceExtensionNames.Size();
		deviceCI.ppEnabledExtensionNames = deviceExtensionNames.Empty() ? nullptr : deviceExtensionNames.Data();

		VkPhysicalDeviceFeatures deviceFeaturesToUse{};
		deviceFeaturesToUse.samplerAnisotropy = VK_FALSE;
		
		result = vkCreateDevice(m_PhysicalDevice, &deviceCI, VULKAN_CPU_ALLOCATOR, &m_Device);
		VULKAN_ASSERT(result, "Failed to create VkDevice.");

		vkGetDeviceQueue(m_Device, m_QueueFamilyIndex, 0, &m_GraphicsQueue);

		if (m_QueueCount > 1)
		{
			vkGetDeviceQueue(m_Device, m_QueueFamilyIndex, 1, &m_PresentQueue);
		}
		else
		{
			m_PresentQueue = m_GraphicsQueue;
		}


		// - Command Pool -

		VkCommandPoolCreateInfo commandPoolCI{};
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCI.queueFamilyIndex = m_QueueFamilyIndex;
		commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		result = vkCreateCommandPool(m_Device, &commandPoolCI, VULKAN_CPU_ALLOCATOR, &m_CommandPool);
		VULKAN_ASSERT(result, "Failed to create VkCommandPool");


		// - Command Buffer -

		m_CommandBuffers.Resize(ImageCount);

		VkCommandBufferAllocateInfo commandBufferInfo{};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferInfo.commandBufferCount = m_CommandBuffers.Size();
		commandBufferInfo.commandPool = m_CommandPool;
		commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		
		result = vkAllocateCommandBuffers(m_Device, &commandBufferInfo, m_CommandBuffers.Data());
		VULKAN_ASSERT(result, "Failed to allocate VkCommandBuffer");


		// - Semaphores -

		VkSemaphoreCreateInfo semaphoreCI{};
		semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		
		m_RenderFinishedSemaphores.Resize(ImageCount);

		for (int i = 0; i < ImageCount; i++)
		{
			VkSemaphore semaphore = VK_NULL_HANDLE;

			result = vkCreateSemaphore(m_Device, &semaphoreCI, VULKAN_CPU_ALLOCATOR, &semaphore);
			VULKAN_ASSERT(result, "Failed to create Render Finished semaphore.");

			m_RenderFinishedSemaphores[i] = semaphore;
		}


		// - Fences -

		VkFenceCreateInfo fenceCI{};
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		m_RenderFinishedFences.Resize(ImageCount);

		for (int i = 0; i < ImageCount; i++)
		{
			VkFence fence = VK_NULL_HANDLE;

			result = vkCreateFence(m_Device, &fenceCI, VULKAN_CPU_ALLOCATOR, &fence);
			VULKAN_ASSERT(result, "Failed to create Render Finished fence.");

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
			VULKAN_ASSERT(result, "Failed to create VkRenderPass.");
		}


		// - Main Graphics Pipeline -

		{
			const FShader* mainShader = Fusion::Shaders::FindShader("Fusion");
			FUSION_ASSERT(mainShader != nullptr, "Failed to find the main Fusion.slang shader!");

			const FShaderModule* vertexShader = mainShader->FindModule(FShaderStage::Vertex);
			const FShaderModule* fragmentShader = mainShader->FindModule(FShaderStage::Fragment);

			m_MainGraphicsPipeline = new FGraphicsPipeline(m_Device);

			VkShaderModuleCreateInfo vertexModuleCI{};
			vertexModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			vertexModuleCI.codeSize = vertexShader->m_SPIRVSize;
			vertexModuleCI.pCode = (const uint32_t*)vertexShader->m_SPIRVData;
			
			result = vkCreateShaderModule(m_Device, &vertexModuleCI, VULKAN_CPU_ALLOCATOR, &m_MainGraphicsPipeline->m_VertexModule);
			VULKAN_ASSERT(result, "Failed to load Vertex Shader.");

			VkShaderModuleCreateInfo fragmentModuleCI{};
			fragmentModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			fragmentModuleCI.codeSize = fragmentShader->m_SPIRVSize;
			fragmentModuleCI.pCode = (const uint32_t*)fragmentShader->m_SPIRVData;

			result = vkCreateShaderModule(m_Device, &fragmentModuleCI, VULKAN_CPU_ALLOCATOR, &m_MainGraphicsPipeline->m_FragmentModule);
			VULKAN_ASSERT(result, "Failed to load Fragment Shader.");

			VkGraphicsPipelineCreateInfo graphicsPipelineCI{};
			graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

			VkPipelineShaderStageCreateInfo stages[2] = {};

			VkPipelineShaderStageCreateInfo& vertexStageCI = stages[0];
			vertexStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertexStageCI.flags = 0;
			vertexStageCI.pName = "main";
			vertexStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertexStageCI.module = m_MainGraphicsPipeline->m_VertexModule;

			VkPipelineShaderStageCreateInfo& fragmentStageCI = stages[1];
			fragmentStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragmentStageCI.flags = 0;
			fragmentStageCI.pName = "main";
			fragmentStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentStageCI.module = m_MainGraphicsPipeline->m_FragmentModule;

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

			vertexInputState.vertexAttributeDescriptionCount = 4;
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
			dynamicState.dynamicStateCount = dynamicStates.size();
			dynamicState.pDynamicStates = dynamicStates.data();

			graphicsPipelineCI.pDynamicState = &dynamicState;

			VkPipelineLayoutCreateInfo pipelineLayoutCI{};
			pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCI.pushConstantRangeCount = 0;

			// Set 0
			{
				VkDescriptorSetLayoutCreateInfo setLayoutCI{};
				setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				setLayoutCI.bindingCount = 0;

				VkDescriptorSetLayout setLayout = nullptr;
				result = vkCreateDescriptorSetLayout(m_Device, &setLayoutCI, VULKAN_CPU_ALLOCATOR, &setLayout);
				VULKAN_ASSERT(result, "Failed to create Set Layout.");
				
				m_MainGraphicsPipeline->m_SetLayouts.Add(setLayout);
			}
			
			// Set 1
			{
				VkDescriptorSetLayoutCreateInfo setLayoutCI{};
				setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				
				FArray<VkDescriptorSetLayoutBinding> bindings{};
				bindings.Add({
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
				VULKAN_ASSERT(result, "Failed to create Set Layout.");

				m_MainGraphicsPipeline->m_SetLayouts.Add(setLayout);
			}

			// Set 2
			{
				VkDescriptorSetLayoutCreateInfo setLayoutCI{};
				setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

				FArray<VkDescriptorSetLayoutBinding> bindings{};
				bindings.Add({
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
				VULKAN_ASSERT(result, "Failed to create Set Layout.");

				m_MainGraphicsPipeline->m_SetLayouts.Add(setLayout);
			}

			pipelineLayoutCI.setLayoutCount = m_MainGraphicsPipeline->m_SetLayouts.Size();
			pipelineLayoutCI.pSetLayouts = m_MainGraphicsPipeline->m_SetLayouts.Data();

			result = vkCreatePipelineLayout(m_Device, &pipelineLayoutCI, VULKAN_CPU_ALLOCATOR, &m_MainGraphicsPipeline->m_PipelineLayout);
			VULKAN_ASSERT(result, "Failed to create Main Pipeline Layout.");

			graphicsPipelineCI.layout = m_MainGraphicsPipeline->m_PipelineLayout;

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

			result = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineCI, VULKAN_CPU_ALLOCATOR, &m_MainGraphicsPipeline->m_Pipeline);
			VULKAN_ASSERT(result, "Failed to create Main Graphics Pipeline.");
		}

		// - Descriptors -

		m_Pool = new FDescriptorPool(this, m_Device);
	}

	void FVulkanRenderBackend::ShutdownVulkan()
	{
		vkDeviceWaitIdle(m_Device);

		for (int i = 0; i < m_DeferredDestruction.Size(); i++)
		{
			m_DeferredDestruction[i].m_Destruction.ExecuteIfBound();
		}
		m_DeferredDestruction.Clear();

		delete m_Pool; m_Pool = nullptr;

		m_SwapChainsByWindowHandle.Clear();

		m_PlatformBackend->SetRenderBackendEventSink(nullptr);

		if (m_RenderPass)
		{
			vkDestroyRenderPass(m_Device, m_RenderPass, VULKAN_CPU_ALLOCATOR);
			m_RenderPass = VK_NULL_HANDLE;
		}

		m_MainGraphicsPipeline = nullptr;

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

	void FVulkanRenderBackend::UpdateAllSwapChains()
	{
		for (auto [windowHandle, swapChain] : m_SwapChainsByWindowHandle)
		{
			CreateOrUpdateSwapChain(windowHandle);
		}
	}

	void FVulkanRenderBackend::CreateOrUpdateSwapChain(FWindowHandle window)
	{
		IntrusivePtr<FSwapChain> swapChain = m_SwapChainsByWindowHandle[window];

		VkResult result = VK_SUCCESS;

		if (swapChain == nullptr)
		{
			swapChain = new FSwapChain(this, m_Device);

			swapChain->m_Surface = FVulkanPlatform::CreateSurface(this, window);

			swapChain->m_ImageAcquiredSemaphores.Resize(ImageCount);

			for (int i = 0; i < ImageCount; i++)
			{
				VkSemaphoreCreateInfo semaphoreCI{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0
				};

				result = vkCreateSemaphore(m_Device, &semaphoreCI, VULKAN_CPU_ALLOCATOR, &swapChain->m_ImageAcquiredSemaphores[i]);
				VULKAN_ASSERT(result, "Failed to create Image Acquired semaphore.");
			}
		}

		swapChain->m_Images.Clear();

		swapChain->m_Window = window;

		VkSwapchainKHR oldSwapChain = swapChain->m_SwapChain;

		VkSurfaceCapabilitiesKHR surfaceCapabilities{};

		result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, swapChain->m_Surface, &surfaceCapabilities);
		VULKAN_ASSERT(result, "Failed to fetch surface capabilities");

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

		swapChain->width = extent.width;
		swapChain->height = extent.height;

		uint32_t presentModesCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, swapChain->m_Surface, &presentModesCount, nullptr);

		FArray<VkPresentModeKHR> presentModes(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, swapChain->m_Surface, &presentModesCount, presentModes.Data());

		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		
		if (presentModes.Contains(VK_PRESENT_MODE_MAILBOX_KHR))
		{
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		}

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
		VULKAN_ASSERT(result, "Failed to create Vulkan SwapChain.");

		uint32_t swapChainImageCount = 0;
		vkGetSwapchainImagesKHR(m_Device, swapChain->m_SwapChain, &swapChainImageCount, nullptr);

		FArray<VkImage> swapChainImages(swapChainImageCount);
		vkGetSwapchainImagesKHR(m_Device, swapChain->m_SwapChain, &swapChainImageCount, swapChainImages.Data());

		for (int i = 0; i < swapChainImageCount; i++)
		{
			IntrusivePtr<FTexture> texture = new FTexture(this, m_Device);

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
			VULKAN_ASSERT(result, "Failed to create Vulkan ImageView for SwapChain.");

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
			frameBufferCI.width = swapChain->width;
			frameBufferCI.height = swapChain->height;
			frameBufferCI.renderPass = m_RenderPass;
			frameBufferCI.layers = 1;

			result = vkCreateFramebuffer(m_Device, &frameBufferCI, VULKAN_CPU_ALLOCATOR, &swapChain->m_FrameBuffers[i]);
			VULKAN_ASSERT(result, "Failed to create FrameBuffer.");
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

		m_SwapChainsByWindowHandle[window] = swapChain;
	}

	void FVulkanRenderBackend::DestroySwapChain(FWindowHandle window)
	{
		if (!m_SwapChainsByWindowHandle.KeyExists(window))
		{
			return;
		}

		IntrusivePtr<FSwapChain> swapChain = m_SwapChainsByWindowHandle[window];

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

	void FVulkanRenderBackend::OnWindowResized(FWindowHandle window, u32 newWidth, u32 newHeight)
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

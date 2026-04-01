#include "VulkanRHIPrivate.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#include "PAL/VulkanPlatform.h"

namespace Fusion
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
	// Graphics Pipeline

	FGraphicsPipeline::FGraphicsPipeline(VkDevice device) : m_Device(device)
	{
	}

	FGraphicsPipeline::~FGraphicsPipeline()
	{
		vkDestroyPipeline(m_Device, m_Pipeline, VULKAN_CPU_ALLOCATOR);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, VULKAN_CPU_ALLOCATOR);

		vkDestroyShaderModule(m_Device, m_VertexModule, VULKAN_CPU_ALLOCATOR);
		vkDestroyShaderModule(m_Device, m_FragmentModule, VULKAN_CPU_ALLOCATOR);
	}

	// -----------------------------------------------------------------
	// Vulkan Render Backend
	

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

	// ------------------------------------------------------------------------------------------------------------
	// - Vulkan
	// ------------------------------------------------------------------------------------------------------------

	void FVulkanRenderBackend::InitializeVulkan()
	{
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
		FUSION_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan instance.");

		if (FVulkanPlatform::IsValidationEnabled())
		{
			result = CreateDebugUtilsMessengerEXT(m_VulkanInstance, &debugCI, VULKAN_CPU_ALLOCATOR, &m_VkMessenger);
			FUSION_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan debug messenger.");
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
		}
		FVulkanPlatform::DestroyTempSurface(m_VulkanInstance, tempSurfaceData);

		FUSION_ASSERT(!m_SurfaceFormats.Empty(), "Failed to find any Surface Formats.");
		FUSION_ASSERT(!m_PresentationModes.Empty(), "Failed to find any Surface Present Modes.");


		// - Queues -

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
		FUSION_ASSERT(queueFamilyCount > 0, "Failed to fetch Queue Family Properties");

		m_QueueFamilyProperties.Resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, m_QueueFamilyProperties.Data());

		m_QueueFamilyIndex = -1;
		float queuePriority = 1.0f;

		for (int i = 0; i < (int)queueFamilyCount; i++)
		{
			if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				m_QueueFamilyIndex = i;
				break;
			}
		}

		FUSION_ASSERT(m_QueueFamilyIndex >= 0, "Failed to find a valid Queue Family.");

		VkDeviceQueueCreateInfo queueCI{};
		queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCI.queueCount = 1;
		queueCI.pQueuePriorities = &queuePriority;
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
		FUSION_ASSERT(result == VK_SUCCESS, "Failed to create VkDevice.");

		vkGetDeviceQueue(m_Device, m_QueueFamilyIndex, 0, &m_GraphicsQueue);

		// - Command Pool -

		VkCommandPoolCreateInfo commandPoolCI{};
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCI.queueFamilyIndex = m_QueueFamilyIndex;

		result = vkCreateCommandPool(m_Device, &commandPoolCI, VULKAN_CPU_ALLOCATOR, &m_CommandPool);
		FUSION_ASSERT(result == VK_SUCCESS, "Failed to create VkCommandPool");

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
			FUSION_ASSERT(result == VK_SUCCESS, "Failed to create VkRenderPass.");
		}

		// - Shader Library -

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
			FUSION_ASSERT(result == VK_SUCCESS, "Failed to load Vertex Shader.");

			VkShaderModuleCreateInfo fragmentModuleCI{};
			fragmentModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			fragmentModuleCI.codeSize = fragmentShader->m_SPIRVSize;
			fragmentModuleCI.pCode = (const uint32_t*)fragmentShader->m_SPIRVData;

			result = vkCreateShaderModule(m_Device, &fragmentModuleCI, VULKAN_CPU_ALLOCATOR, &m_MainGraphicsPipeline->m_FragmentModule);
			FUSION_ASSERT(result == VK_SUCCESS, "Failed to load Fragment Shader.");

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
			pipelineLayoutCI.setLayoutCount = 0;
			pipelineLayoutCI.pushConstantRangeCount = 0;

			result = vkCreatePipelineLayout(m_Device, &pipelineLayoutCI, VULKAN_CPU_ALLOCATOR, &m_MainGraphicsPipeline->m_PipelineLayout);
			FUSION_ASSERT(result == VK_SUCCESS, "Failed to create Main Pipeline Layout.");

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
			FUSION_ASSERT(result == VK_SUCCESS, "Failed to create Main Graphics Pipeline.");
		}
	}

	void FVulkanRenderBackend::ShutdownVulkan()
	{
		if (m_RenderPass)
		{
			vkDestroyRenderPass(m_Device, m_RenderPass, VULKAN_CPU_ALLOCATOR);
			m_RenderPass = VK_NULL_HANDLE;
		}

		m_MainGraphicsPipeline = nullptr;

		if (m_CommandPool)
		{
			vkDestroyCommandPool(m_Device, m_CommandPool, VULKAN_CPU_ALLOCATOR);
			m_CommandPool = VK_NULL_HANDLE;
		}

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
}

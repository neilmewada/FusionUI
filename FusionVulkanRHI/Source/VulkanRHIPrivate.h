#pragma once

#include "Fusion/VulkanRHI.h"

#define VULKAN_CHECK(vkResult, message) FUSION_ASSERT(vkResult == VK_SUCCESS, message)

#define VULKAN_CPU_ALLOCATOR VK_NULL_HANDLE

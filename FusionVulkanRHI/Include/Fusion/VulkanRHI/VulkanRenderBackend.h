#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    
    class FUSIONVULKANRHI_API FVulkanRenderBackend : public IFRenderBackend
    {
    public:

        void InitializeShaders() override;

        bool IsInitialized(FInstanceHandle instance) override
        {
            return true;
        }

        bool InitializeInstance(FInstanceHandle instance) override;

        void ShutdownInstance(FInstanceHandle instance) override;

	private:

		VkInstance m_VulkanInstance = VK_NULL_HANDLE;


    };

} // namespace Fusion

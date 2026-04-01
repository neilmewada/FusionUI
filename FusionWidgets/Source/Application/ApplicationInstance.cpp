#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

	bool FApplicationInstance::Initialize(const FApplicationInstanceDesc& desc)
	{
		m_PlatformBackend = desc.platformBackend;
		
#if FUSION_USE_SDL3
		if (!m_PlatformBackend)
		{
			m_PlatformBackendAllocated = true;
			m_PlatformBackend = new FSDL3PlatformBackend();
		}
#endif

		if (!m_PlatformBackend)
		{
			FUSION_LOG_ERROR("Backend", "No platform backend specified and no default available.");
			return false;
		}

		m_RenderBackend = desc.renderBackend;

#if FUSION_USE_VULKAN
		if (!m_RenderBackend)
		{
			m_RenderBackendAllocated = true;
			m_RenderBackend = new FVulkanRenderBackend(m_PlatformBackend);
		}
#endif

		if (!m_RenderBackend)
		{
			FUSION_LOG_ERROR("Backend", "No render backend specified and no default available.");
			return false;
		}

		m_InstanceHandle = m_PlatformBackend->InitializeInstance();

		if (m_InstanceHandle.IsNull())
		{
			FUSION_LOG_ERROR("Backend", "Failed to initialize platform backend instance.");
			return false;
		}

		m_PlatformBackend->SetEventSink(m_InstanceHandle, this);

		m_RenderBackend->InitializeInstance(m_InstanceHandle);

		return true;
	}

	void FApplicationInstance::Shutdown()
	{
		if (m_RenderBackend)
		{
			m_RenderBackend->ShutdownInstance(m_InstanceHandle);
		}

		if (m_PlatformBackend)
		{
			m_PlatformBackend->ShutdownInstance(m_InstanceHandle);
		}

		m_InstanceHandle = FInstanceHandle::NullValue;

		if (m_PlatformBackendAllocated)
		{
			delete m_PlatformBackend; m_PlatformBackend = nullptr;
		}

		if (m_RenderBackendAllocated)
		{
			delete m_RenderBackend; m_RenderBackend = nullptr;
		}

		m_PlatformBackendAllocated = m_RenderBackendAllocated = false;
	}

	void FApplicationInstance::OnWindowDestroyed([[maybe_unused]] FWindowHandle window)
	{
		
	}

} // namespace Fusion

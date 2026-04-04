#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	FApplication::FApplication([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
	{
		m_MainApplication = new FApplicationInstance("FusionApplication");
	}

	int FApplication::Run()
	{
		if (m_PlatformBackend == nullptr)
		{
#if FUSION_USE_SDL3
			m_PlatformBackend = new FSDL3PlatformBackend();
#else
			FUSION_LOG_ERROR("Backend", "No platform backend specified and no default available.");
			return -1;
#endif
		}

		if (m_RenderBackend == nullptr)
		{
#if FUSION_USE_VULKAN
			m_RenderBackend = new Vulkan::FVulkanRenderBackend(m_PlatformBackend);
#else
			FUSION_LOG_ERROR("Backend", "No render backend specified and no default available.");
			return -1;
#endif
		}

		int returnCode = 0;

		// TODO: This is all sample code for now

		m_MainApplication->Initialize({
			m_PlatformBackend,
			m_RenderBackend
			});

		FInstanceHandle instanceHandle = m_MainApplication->GetInstanceHandle();

		if (m_MainWindow)
		{
			FWindowHandle windowHandle = m_PlatformBackend->CreateWindow(instanceHandle, "Fusion 1", m_InitialWindowSize.width, m_InitialWindowSize.height, {
				.maximised = false,
				.fullscreen = false,
				.resizable = true,
				.hidden = false,
				.borderless = false,
				.openCentered = true,
				.windowFlags = FPlatformWindowFlags::DestroyOnClose
			});

			Ref<FNativeSurface> nativeSurface = m_MainApplication->CreateNativeSurfaceForWindow(windowHandle);

			nativeSurface->SetOwningWidget(m_MainWindow);
		}

		m_PlatformBackend->SetContinuousResizeTick([this]
		{
			m_MainApplication->Tick();

			m_RenderBackend->RenderTick();
		});

		m_MainWindow = nullptr;

		while (!m_PlatformBackend->IsUserRequestingExit())
		{
			m_PlatformBackend->PumpEvents();

			m_PlatformBackend->Tick();

			m_MainApplication->Tick();

			m_RenderBackend->RenderTick();
		}

		m_MainApplication->Shutdown();

		delete m_RenderBackend; m_RenderBackend = nullptr;
		delete m_PlatformBackend; m_PlatformBackend = nullptr;

		return returnCode;
	}

} // namespace Fusion

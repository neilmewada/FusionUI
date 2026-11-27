#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	FNativeSurface::FNativeSurface(FWindowHandle windowHandle) : m_WindowHandle(windowHandle)
	{
		
	}

	void FNativeSurface::Initialize()
	{
		Super::Initialize();

		if (Ref<FApplicationInstance> application = GetApplication())
		{
			m_RenderTarget = application->AcquireWindowRenderTarget(m_WindowHandle);

			m_DpiScale = application->GetDpiScaleForWindow(m_WindowHandle);

			FVec2i pixelSize = application->GetWindowSizeInPixels(m_WindowHandle);

			m_AvailableSize = FVec2((float)pixelSize.x, (float)pixelSize.y) / m_DpiScale;
			m_PixelSize = pixelSize.ToVec2();
		}
	}

	void FNativeSurface::Shutdown()
	{
		Super::Shutdown();
	}

    ETitleBarStyle FNativeSurface::GetNativeTitleBarStyle() const
    {
	    if (Ref<FApplicationInstance> application = GetApplication())
	    {
	        return application->GetWindowTitleBarStyle(m_WindowHandle);
	    }
        return FSurface::GetNativeTitleBarStyle();
    }

    FVec2 FNativeSurface::WindowToSurfacePoint(FVec2 position)
	{
		// `position` is already relative to this window, in logical window pixels:
		// the platform backend reports SDL_GetMouseState() coordinates, and
		// m_AvailableSize is the window size in the same units
		// (pixel size / dpi scale). Nothing to convert.
		return position;
	}

	void FNativeSurface::RequestTextInput()
	{
		if (Ref<FApplicationInstance> app = GetApplication())
			app->GetPlatformBackend()->StartTextInput(m_WindowHandle);
	}

	void FNativeSurface::ReleaseTextInput()
	{
		if (Ref<FApplicationInstance> app = GetApplication())
			app->GetPlatformBackend()->StopTextInput(m_WindowHandle);
	}

	void FNativeSurface::OnWindowResized()
	{
		if (Ref<FApplicationInstance> application = GetApplication())
		{
			FVec2i pixelSize = application->GetWindowSizeInPixels(m_WindowHandle);

			FVec2 availableSize = FVec2((f32)pixelSize.x, (f32)pixelSize.y) / m_DpiScale;

			if (m_AvailableSize != availableSize)
			{
				m_AvailableSize = availableSize;
				m_PixelSize = pixelSize.ToVec2();

				OnSurfaceResize();
			}
		}
	}

} // namespace Fusion

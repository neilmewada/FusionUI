#include "Fusion/Widgets.h"

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

	FVec2 FNativeSurface::ScreenToSurfacePoint(FVec2 position)
	{
		Ref<FApplicationInstance> application = GetApplication();
		if (!application)
			return position;
		
#if FUSION_PLATFORM_MAC
		return (position - application->GetWindowPosition(m_WindowHandle).ToVec2());
#else
		return (position - application->GetWindowPosition(m_WindowHandle).ToVec2()) / GetDpiScale();
#endif
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

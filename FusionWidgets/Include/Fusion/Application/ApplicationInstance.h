#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	class FWidget;
	class FSurface;
	class FNativeSurface;
	class FStyleSheet;

	struct FApplicationInstanceDesc
	{
		IFPlatformBackend* PlatformBackend = nullptr;
		IFRenderBackend*   RenderBackend   = nullptr;
	};

	class FUSIONWIDGETS_API FApplicationInstance : public FObject, public IFPlatformEventSink
	{
		FUSION_CLASS(FApplicationInstance, FObject)
	public:

		FApplicationInstance(FName name) : FObject(name) {}

		// - Public API -

		f32    GetDpiScaleForWindow(FWindowHandle windowHandle);
		FVec2i GetWindowSizeInPixels(FWindowHandle windowHandle);

		// - Input State -

		FVec2 GetScreenMousePos()     const { return m_ScreenMousePos; }
		FVec2 GetPrevScreenMousePos() const { return m_PrevScreenMousePos; }
		FVec2 GetMouseWheelDelta()    const { return m_WheelDelta; }

		bool IsKeyDown(EKeyCode key)              { return m_PlatformBackend->IsKeyDown(key); }
		bool IsKeyUp(EKeyCode key)                { return m_PlatformBackend->IsKeyUp(key); }
		bool IsMouseButtonDown(EMouseButton btn)  { return m_PlatformBackend->IsMouseButtonDown(btn); }
		bool IsMouseButtonUp(EMouseButton btn)    { return m_PlatformBackend->IsMouseButtonUp(btn); }
		bool IsMouseButtonHeld(EMouseButton btn)  { return m_PlatformBackend->IsMouseButtonHeld(btn); }
		int  GetMouseButtonClicks(EMouseButton btn) { return m_PlatformBackend->GetMouseButtonClicks(btn); }
		bool         TestModifiers(EKeyModifier modifier) { return m_PlatformBackend->TestModifiers(modifier); }
		EKeyModifier GetKeyModifiers()                   { return m_PlatformBackend->GetModifierStates(); }

		// - Window API -

		FVec2i GetWindowPosition(FWindowHandle window) { return m_PlatformBackend->GetWindowPosition(window); }

		// - Lifecycle -

		bool Initialize(const FApplicationInstanceDesc& desc);
		void Shutdown();
		void Tick();

		// - Surface -

		Ref<FNativeSurface> CreateNativeSurfaceForWindow(FWindowHandle window);

		void FocusSurface(Ref<FSurface> surface);
		void UnfocusSurface(Ref<FSurface> surface);

		FInstanceHandle            GetInstanceHandle()      const { return m_InstanceHandle; }
		FRenderBackendCapabilities GetRenderCapabilities()  const { return m_RenderCapabilities; }

		// - Render Target -

		FRenderTargetHandle AcquireWindowRenderTarget(FWindowHandle window);
		void                ReleaseRenderTarget(FRenderTargetHandle renderTarget);
		void                SubmitSnapshot(FRenderTargetHandle renderTarget, IntrusivePtr<FRenderSnapshot> snapshot);

		// - Style -

		Ref<FStyleSheet> GetStyleSheet() const { return m_RootStyleSheet; }

		void SetRootStyleSheet(Ref<FStyleSheet> styleSheet);

		void RefreshStyleRecursively();

	protected:

		void OnWindowDestroyed(FWindowHandle window) override;
		void OnWindowResized(FWindowHandle window, u32 newWidth, u32 newHeight) override;
		void OnWindowMaximized(FWindowHandle window) override;
		void OnWindowMinimized(FWindowHandle window) override;
		void OnWindowRestored(FWindowHandle window) override;

		void OnWindowKeyboardFocusChanged(FWindowHandle window, bool gotFocus) override;
		void OnWindowMouseFocusChanged(FWindowHandle window, bool gotFocus) override;

		void NotifyWindowResize(FWindowHandle window);

		FInstanceHandle m_InstanceHandle = FInstanceHandle::NullValue;

		FArray<Ref<FSurface>>                        m_Surfaces;
		FHashMap<FWindowHandle, Ref<FNativeSurface>> m_NativeSurfacesByWindow;

		WeakRef<FSurface> m_CurFocusSurface;
		WeakRef<FSurface> m_FocusSurface;

		Ref<FStyleSheet> m_RootStyleSheet;

		FVec2 m_ScreenMousePos;
		FVec2 m_PrevScreenMousePos;
		FVec2 m_WheelDelta;

		bool m_IsFirstTick = true;

	private:

		bool               m_PlatformBackendAllocated = false;
		IFPlatformBackend* m_PlatformBackend          = nullptr;

		bool             m_RenderBackendAllocated = false;
		IFRenderBackend* m_RenderBackend          = nullptr;

		FRenderBackendCapabilities m_RenderCapabilities{};
	};

} // namespace Fusion

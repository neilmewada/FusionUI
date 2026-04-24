#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT


namespace Fusion
{
	class FImageAtlas;
	class FFontAtlas;
	class FAnimation;
	class FWidget;
	class FSurface;
	class FNativeSurface;
	class FTheme;
	class FTimer;

	FUSION_SIGNAL_TYPE(FVoidSignal);

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

		IFPlatformBackend* GetPlatformBackend() const { return m_PlatformBackend; }
		IFRenderBackend*   GetRenderBackend()   const { return m_RenderBackend; }

		// - Input State -

		FVec2 GetScreenMousePos()     const { return m_ScreenMousePos; }
		FVec2 GetPrevScreenMousePos() const { return m_PrevScreenMousePos; }
		FVec2 GetMouseWheelDelta()    const { return m_WheelDelta; }

		bool 		 IsKeyDown(EKeyCode key)                { return m_PlatformBackend->IsKeyDown(key); }
		bool 		 IsKeyUp(EKeyCode key)                  { return m_PlatformBackend->IsKeyUp(key); }
		bool 		 IsMouseButtonDown(EMouseButton btn)    { return m_PlatformBackend->IsMouseButtonDown(btn); }
		bool 		 IsMouseButtonUp(EMouseButton btn)      { return m_PlatformBackend->IsMouseButtonUp(btn); }
		bool 		 IsMouseButtonHeld(EMouseButton btn)    { return m_PlatformBackend->IsMouseButtonHeld(btn); }
		int  		 GetMouseButtonClicks(EMouseButton btn) { return m_PlatformBackend->GetMouseButtonClicks(btn); }
		bool         TestModifiers(EKeyModifier modifier)   { return m_PlatformBackend->TestModifiers(modifier); }
		EKeyModifier GetKeyModifiers()                      { return m_PlatformBackend->GetModifierStates(); }

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

		// - Theme -

		Ref<FTheme> GetTheme() const { return m_RootTheme; }

		void SetRootTheme(Ref<FTheme> styleSheet);

		void RefreshStyleRecursively();

		// - Animation -

		void PlayAnimation(Ref<FAnimation> animation, Ref<FObject> owner, FName slot);

		void TerminateAnimation(Ref<FObject> owner, FName slot, bool complete = false);

		void TerminateAllAnimations(Ref<FObject> owner, bool complete = false);

		// - Atlas -

		Ref<FFontAtlas>  GetFontAtlas()  const { return m_FontAtlas; }
		Ref<FImageAtlas> GetImageAtlas() const { return m_ImageAtlas; }

		// - Timer -

		void RegisterTimer(Ref<FTimer> timer);
		void DeregisterTimer(Ref<FTimer> timer);

		// - Cursor -
		
		void SetActiveCursor(FCursor cursor);

		void PushCursorOverride(const FCursor& cursor);
		void PopCursorOverride();

	protected:

		void ApplyCursor();

		void OnWindowDestroyed(FWindowHandle window) override;
		void OnWindowResized(FWindowHandle window, u32 newWidth, u32 newHeight) override;
		void OnWindowMaximized(FWindowHandle window) override;
		void OnWindowMinimized(FWindowHandle window) override;
		void OnWindowRestored(FWindowHandle window) override;

		void OnWindowKeyboardFocusChanged(FWindowHandle window, bool gotFocus) override;
		void OnWindowMouseFocusChanged(FWindowHandle window, bool gotFocus) override;

		void OnWindowDisplayChanged(FWindowHandle window, FDisplayId displayId) override;

		void NotifyWindowResize(FWindowHandle window);

		FInstanceHandle m_InstanceHandle = FInstanceHandle::NullValue;

		TArray<Ref<FSurface>>                        m_Surfaces;
		THashMap<FWindowHandle, Ref<FNativeSurface>> m_NativeSurfacesByWindow;

		WeakRef<FSurface> m_CurFocusSurface;
		WeakRef<FSurface> m_FocusSurface;

		Ref<FTheme> m_RootTheme;

		FVec2 m_ScreenMousePos;
		FVec2 m_PrevScreenMousePos;
		FVec2 m_WheelDelta;

		bool m_IsFirstTick = true;
		f32 m_DeltaTime = 0.0f;
		std::chrono::steady_clock::time_point m_PreviousTime = std::chrono::steady_clock::now();

		THashMap<FUuid, THashMap<FName, Ref<FAnimation>>> m_AnimationSlotsByWidget;

		// Flat list of (ownerUuid, slot) pairs queued for removal this tick
		TArray<TPair<FUuid, FName>> m_AnimationsToDestroy;

		// - Atlases

		Ref<FFontAtlas> m_FontAtlas;
		Ref<FImageAtlas> m_ImageAtlas;

		TArray<WeakRef<FTimer>> m_Timers;

		// - Cursor -

		FCursor m_ActiveCursor;
		TArray<FCursor> m_CursorOverrideStack;

	private:

		bool               m_PlatformBackendAllocated = false;
		IFPlatformBackend* m_PlatformBackend          = nullptr;

		bool             m_RenderBackendAllocated = false;
		IFRenderBackend* m_RenderBackend          = nullptr;

		FRenderBackendCapabilities m_RenderCapabilities{};
	};

} // namespace Fusion

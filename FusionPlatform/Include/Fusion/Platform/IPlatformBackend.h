#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    enum class ETitleBarStyle
    {
        Default = 0,
        NoTitleBar,
        //! @brief Available only on macOS. Falls back to NoTitleBar on other platforms.
        TransparentWithNativeControls,
        //! @brief Available only on macOS. Falls back to NoTitleBar on other platforms.
        ToolBarWithNativeControls,
    };
    FUSION_ENUM_CLASS(ETitleBarStyle);

    struct FNativeChromeMetrics
    {
        // Bounding rect of native window controls (traffic lights) in window-local POINTS
        // (top-left origin, not pixels). All zeros on platforms with no native controls.
        FRect  nativeControlsRect;

        // Height of any opaque native titlebar/toolbar above your content.
        // 0 when FullSizeContentView is used — you own the full window area.
        float  titlebarHeight = 0.0f;
    };

    enum class EPlatformWindowFlags
    {
        None = 0,
        ToolTip = FUSION_BIT(0),
        PopupMenu = FUSION_BIT(1),
        Utility = FUSION_BIT(2),
        DestroyOnClose = FUSION_BIT(3),
    };
    FUSION_ENUM_CLASS_FLAGS(EPlatformWindowFlags);

    struct FPlatformWindowInfo
    {
        bool maximised = false;
        bool fullscreen = false;
        bool resizable = true;
        bool hidden = false;
        ETitleBarStyle titleBarStyle = ETitleBarStyle::Default;

		FDisplayId displayId = FDisplayId::NullValue;
        bool openCentered = true;
        FVec2i openPos = FVec2i();
        EPlatformWindowFlags windowFlags = EPlatformWindowFlags::DestroyOnClose;
    };

    struct FPlatformCapabilities
    {
        bool SupportsMultipleNativeSurface = true;
    };

    class FUSIONPLATFORM_API IFPlatformBackend
	{
    public:

        virtual ~IFPlatformBackend() = default;

        virtual FPlatformCapabilities GetCapabilities() = 0;

		// - Lifecycle -

		virtual bool IsInitialized(FInstanceHandle instance) = 0;

        virtual FInstanceHandle InitializeInstance() = 0;

        virtual void Tick() = 0;

        virtual void ShutdownInstance(FInstanceHandle instance) = 0;

		virtual bool IsUserRequestingExit() = 0;

        virtual void SetContinuousResizeTick(const FDelegate<void()>& tick) {}

        virtual void StartTextInput(FWindowHandle window) {}
        virtual void StopTextInput(FWindowHandle window) {}

        // - Native Handles -

        virtual void* GetNativeWindowHandle(FWindowHandle handle) = 0;

        virtual void* GetNativeDisplayHandle(FWindowHandle handle) { return nullptr; }

		// - Events -

        virtual void PumpEvents() = 0;

		virtual void SetInstanceEventSink(FInstanceHandle instance, IFPlatformEventSink* eventSink) = 0;

        virtual void RegisterEventSink(IFPlatformEventSink* eventSink) = 0;
        virtual void DeregisterEventSink(IFPlatformEventSink* eventSink) = 0;

        virtual FVec2 GetGlobalMousePosition() = 0;
        virtual FVec2 GetMouseWheelDelta() = 0;

        virtual bool IsKeyDown(EKeyCode key) = 0;

        virtual bool IsKeyHeld(EKeyCode key) = 0;

        virtual bool IsKeyUp(EKeyCode key) = 0;

        // Returns all keys that transitioned down this tick (excludes repeats).
        virtual TArray<EKeyCode> GetKeysDownThisTick() { return {}; }

        // Returns all keys that transitioned up this tick.
        virtual TArray<EKeyCode> GetKeysUpThisTick() { return {}; }

        // Returns UTF-8 text produced by SDL_EVENT_TEXT_INPUT this tick (may be empty).
        virtual FString GetTextInputThisTick() { return {}; }

        virtual bool IsMouseButtonDown(EMouseButton mouseButton) = 0;

        virtual bool IsMouseButtonUp(EMouseButton mouseButton) = 0;

        virtual int GetMouseButtonClicks(EMouseButton mouseButton) = 0;

        virtual bool IsMouseButtonHeld(EMouseButton mouseButton) = 0;

        //! @brief Tests for presence of ALL of the given modifiers
        virtual bool TestModifiers(EKeyModifier modifier) = 0;

        //! @brief Returns the full current modifier bitmask
        virtual EKeyModifier GetModifierStates() = 0;

		// - Window Management -

		virtual FWindowHandle CreateWindow(FInstanceHandle instance, const FString& title, u32 width, u32 height, const FPlatformWindowInfo& info) = 0;

		virtual void DestroyWindow(FWindowHandle window) = 0;

        virtual ETitleBarStyle GetWindowTitleBarStyle(FWindowHandle window) = 0;

        virtual FVec2i GetWindowSizeInPixels(FWindowHandle window) = 0;

        virtual FVec2i GetWindowSize(FWindowHandle window) = 0;

        virtual FVec2i GetWindowPosition(FWindowHandle window) = 0;

        virtual f32 GetDpiScaleForWindow(FWindowHandle window) = 0;

        virtual void SetSystemCursor(ESystemCursor cursor) = 0;

	};
    
} // namespace Fusion

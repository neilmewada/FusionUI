#pragma once
#include <chrono>

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

	struct FSDL3InstanceData
    {
        FInstanceHandle instance = 0;
		IFPlatformEventSink* eventSink = nullptr;
	};

    struct FSDL3InputState
    {
		FWindowHandle windowHandle = FWindowHandle::NullValue;
        FVec2 globalMousePosition{};
        FVec2 mousePosition{};
        FVec2 mouseDelta{};
        FVec2 wheelDelta{};
        u64 curTime = 0;

        FHashMap<EKeyCode, bool> keyStates{};
        //HashMap<FKeyCode, Internal::FKeyStateDelayed> keyStatesDelayed{};
        FHashMap<EMouseButton, int> mouseButtonStates{};

        // Per-Tick changes
        FHashMap<EKeyCode, bool> stateChangesThisTick{};
        TArray<EKeyCode> keyRepeatThisTick{};
        FHashMap<EMouseButton, int> mouseButtonStateChanges{};

        EKeyModifier modifierStates{};
        FString textInput{};
    };

    class FUSIONSDL3PLATFORM_API FSDL3PlatformBackend : public IFPlatformBackend
    {
    public:

        FPlatformCapabilities GetCapabilities() override;

		// - Lifecycle -

        bool IsInitialized(FInstanceHandle instance) override;

		FInstanceHandle InitializeInstance() override;

        void ShutdownInstance(FInstanceHandle instance) override;

		void Tick() override;

        bool IsUserRequestingExit() override { return m_UserRequestedExit; }

        void* GetNativeWindowHandle(FWindowHandle handle) override;

		// - Events -

        void SetContinuousResizeTick(const FDelegate<void()>& tick) override;

        void PumpEvents() override;

        void SetInstanceEventSink(FInstanceHandle instance, IFPlatformEventSink* eventSink) override;

        void RegisterEventSink(IFPlatformEventSink* eventSink) override;

        void DeregisterEventSink(IFPlatformEventSink* eventSink) override;

        FVec2 GetGlobalMousePosition() override;

        FVec2 GetMouseWheelDelta() override;

        bool IsKeyDown(EKeyCode key) override;
        bool IsKeyUp(EKeyCode key) override;
        bool IsKeyHeld(EKeyCode key) override;

        TArray<EKeyCode> GetKeysDownThisTick() override;
        TArray<EKeyCode> GetKeysUpThisTick()   override;
        FString          GetTextInputThisTick() override;

        bool IsMouseButtonDown(EMouseButton mouseButton) override;
        bool IsMouseButtonUp(EMouseButton mouseButton) override;
        bool IsMouseButtonHeld(EMouseButton mouseButton) override;
        int  GetMouseButtonClicks(EMouseButton mouseButton) override;

        bool         TestModifiers(EKeyModifier modifier) override;
        EKeyModifier GetModifierStates() override;

        // - Windowing -

        FWindowHandle CreateWindow(FInstanceHandle instance, const FString& title, u32 width, u32 height, const FPlatformWindowInfo& info) override;

		void DestroyWindow(FWindowHandle window) override;

        FVec2i GetWindowSizeInPixels(FWindowHandle window) override;

        FVec2i GetWindowSize(FWindowHandle window) override;

        FVec2i GetWindowPosition(FWindowHandle window) override;

		f32 GetDpiScaleForWindow(FWindowHandle window) override;

        void SetSystemCursor(ESystemCursor cursor) override;

        void StartTextInput(FWindowHandle window) override;
        void StopTextInput(FWindowHandle window) override;

	protected:

        void ProcessWindowEvents(SDL_Event& event);

        void ProcessInputEvents(SDL_Event& event);

        void ProcessWindowResizeEvent(FWindowHandle windowHandle);

        FHashMap<FInstanceHandle, FSDL3InstanceData> m_Instances;

		FHashMap<FWindowHandle, FSDL3PlatformWindow*> m_WindowsByHandle;

		FHashMap<FUuid, bool> m_Displays;

		bool m_IsInitialized = false;
        bool m_InitFailed = false;
		bool m_UserRequestedExit = false;

        FInstanceHandle m_InstanceCounter = FInstanceHandle::NullValue;

        FHashSet<IFPlatformEventSink*> m_RegisteredSinks{};

        FDelegate<void()> m_ContinuousResizeTick{};

        FHashMap<ESystemCursor, SDL_Cursor*> m_SystemCursors;

        friend bool SDLEventWatch(void* userdata, SDL_Event* event);

        // - Input State -

        std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime = std::chrono::high_resolution_clock::now();
        u64 m_CurTime = 0;

        FWindowHandle m_InputWindowHandle = FWindowHandle::NullValue;
        FVec2 m_GlobalMousePosition{};
        FVec2 m_MousePosition{};
        FVec2 m_PrevGlobalMousePosition{};
        //Vec2i mouseDelta{};
        FVec2 m_WheelDelta{};
        FString m_TextInput{};

        TArray<u64> m_FocusGainedWindows{};
        TArray<u64> m_FocusLostWindows{};

        FHashMap<EKeyCode, bool> m_KeyStates{};
        //HashMap<FKeyCode, Internal::KeyStateDelayed> keyStatesDelayed{};
        FHashMap<EMouseButton, int> m_MouseButtonStates{};

        // Per-Tick changes
        FHashMap<EKeyCode, bool> m_StateChangesThisTick{};
        TArray<EKeyCode> m_KeyRepeatThisTick{};
        FHashMap<EMouseButton, int> m_MouseButtonStateChanges{};

        EKeyModifier m_ModifierStates{};

        FSDL3InputState m_InputState{};
    };
    
} // namespace Fusion

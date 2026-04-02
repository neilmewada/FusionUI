#pragma once
#include <chrono>

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

        FHashMap<FKeyCode, bool> keyStates{};
        //HashMap<FKeyCode, Internal::FKeyStateDelayed> keyStatesDelayed{};
        FHashMap<FMouseButton, int> mouseButtonStates{};

        // Per-Tick changes
        FHashMap<FKeyCode, bool> stateChangesThisTick{};
        FHashMap<FMouseButton, int> mouseButtonStateChanges{};

        FKeyModifier modifierStates{};
    };

    class FUSIONSDL3PLATFORM_API FSDL3PlatformBackend : public IFPlatformBackend
    {
    public:

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

        void SetEventSink(FInstanceHandle instance, IFPlatformEventSink* eventSink) override;

        void SetRenderBackendEventSink(IFPlatformEventSink* genericEventSink) override
        {
            m_RenderBackendEventSink = genericEventSink;
        }

        // - Windowing -

        FWindowHandle CreateWindow(FInstanceHandle instance, const FString& title, u32 width, u32 height, const FPlatformWindowInfo& info) override;

		void DestroyWindow(FWindowHandle window) override;

        FVec2i GetWindowSizeInPixels(FWindowHandle window) override;

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

        IFPlatformEventSink* m_RenderBackendEventSink = nullptr;

        FDelegate<void()> m_ContinuousResizeTick{};

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

        FArray<u64> m_FocusGainedWindows{};
        FArray<u64> m_FocusLostWindows{};

        FHashMap<FKeyCode, bool> m_KeyStates{};
        //HashMap<FKeyCode, Internal::KeyStateDelayed> keyStatesDelayed{};
        FHashMap<FMouseButton, int> m_MouseButtonStates{};

        // Per-Tick changes
        FHashMap<FKeyCode, bool> m_StateChangesThisTick{};
        FHashMap<FMouseButton, int> m_MouseButtonStateChanges{};

        FKeyModifier m_ModifierStates{};

        FSDL3InputState m_InputState{};
    };
    
} // namespace Fusion

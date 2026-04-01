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

        HashMap<FKeyCode, bool> keyStates{};
        //HashMap<FKeyCode, Internal::FKeyStateDelayed> keyStatesDelayed{};
        HashMap<FMouseButton, int> mouseButtonStates{};

        // Per-Tick changes
        HashMap<FKeyCode, bool> stateChangesThisTick{};
        HashMap<FMouseButton, int> mouseButtonStateChanges{};

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

        void PumpEvents() override;

        void SetEventSink(FInstanceHandle instance, IFPlatformEventSink* eventSink) override;

        FWindowHandle CreateWindow(FInstanceHandle instance, const FString& title, u32 width, u32 height, const FPlatformWindowInfo& info) override;

		void DestroyWindow(FWindowHandle window) override;

	protected:

        void ProcessWindowEvents(SDL_Event& event);

        void ProcessInputEvents(SDL_Event& event);

        void ProcessWindowResizeEvent(FSDL3PlatformWindow* window);

        HashMap<FInstanceHandle, FSDL3InstanceData> instances;

		HashMap<FWindowHandle, FSDL3PlatformWindow*> windowsByHandle;

		HashMap<FUuid, bool> m_Displays;

		bool m_IsInitialized = false;
        bool m_InitFailed = false;
		bool m_UserRequestedExit = false;

        FInstanceHandle instanceCounter = FInstanceHandle::NullValue;


        // - Input -

        std::chrono::time_point<std::chrono::high_resolution_clock> startTime = std::chrono::high_resolution_clock::now();
        u64 curTime = 0;

        FWindowHandle inputWindowHandle = FWindowHandle::NullValue;
        FVec2 globalMousePosition{};
        FVec2 mousePosition{};
        FVec2 prevGlobalMousePosition{};
        //Vec2i mouseDelta{};
        FVec2 wheelDelta{};

        FArray<u64> focusGainedWindows{};
        FArray<u64> focusLostWindows{};

        HashMap<FKeyCode, bool> keyStates{};
        //HashMap<FKeyCode, Internal::KeyStateDelayed> keyStatesDelayed{};
        HashMap<FMouseButton, int> mouseButtonStates{};

        // Per-Tick changes
        HashMap<FKeyCode, bool> stateChangesThisTick{};
        HashMap<FMouseButton, int> mouseButtonStateChanges{};

        FKeyModifier modifierStates{};


        FSDL3InputState inputState{};
    };
    
} // namespace Fusion

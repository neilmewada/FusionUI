#include "Fusion/SDL3Platform.h"

namespace Fusion
{
	bool FSDL3PlatformBackend::IsInitialized(FInstanceHandle instance)
	{
		return instances.KeyExists(instance);
	}

	FInstanceHandle FSDL3PlatformBackend::InitializeInstance()
	{
		if (m_InitFailed)
		{
			return 0;
		}

		if (!m_IsInitialized)
		{
			SDL_SetHint(SDL_HINT_APP_NAME, "SDL3Application");

			SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

#if FUSION_PLATFORM_MAC
			SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "1");
#endif

			SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "1");
			SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");

			if (!SDL_Init(SDL_INIT_VIDEO))
			{
				m_InitFailed = true;
				return false;
			}

			m_IsInitialized = true;
			m_UserRequestedExit = false;
		}

		if (instanceCounter == FInstanceHandle::NullValue)
		{
			instanceCounter = 0;
		}
		else
		{
			instanceCounter = instanceCounter.Get() + 1;
		}

		FInstanceHandle handle = instanceCounter;

		instances[handle] = FSDL3InstanceData{
			handle
		};

		return handle;
	}

	void FSDL3PlatformBackend::ShutdownInstance(FInstanceHandle instance)
	{
		if (!instances.KeyExists(instance))
		{
			return;
		}

		instances.Remove(instance);

		if (instances.IsEmpty())
		{
			m_IsInitialized = false;
			SDL_Quit();
		}
	}

	void FSDL3PlatformBackend::Tick()
	{
		auto now = std::chrono::high_resolution_clock::now();
		curTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

		SDL_GetMouseState(&mousePosition.x, &mousePosition.y);
		SDL_GetGlobalMouseState(&globalMousePosition.x, &globalMousePosition.y);

		inputState.windowHandle = inputWindowHandle;
		inputState.stateChangesThisTick = stateChangesThisTick;
		inputState.keyStates = keyStates;
		inputState.modifierStates = modifierStates;
		//inputState.keyStatesDelayed = keyStatesDelayed;
		inputState.mouseButtonStates = mouseButtonStates;
		inputState.mouseButtonStateChanges = mouseButtonStateChanges;
		inputState.mousePosition = mousePosition;
		inputState.globalMousePosition = globalMousePosition;
		inputState.mouseDelta = globalMousePosition - prevGlobalMousePosition;
		inputState.wheelDelta = wheelDelta;
		inputState.curTime = curTime;

		// Reset temp values
		prevGlobalMousePosition = globalMousePosition;
		wheelDelta = FVec2();

		stateChangesThisTick.Clear();
		mouseButtonStateChanges.Clear();
		focusGainedWindows.Clear();
		focusLostWindows.Clear();
	}

	void* FSDL3PlatformBackend::GetNativeWindowHandle(FWindowHandle handle)
	{
		if (!windowsByHandle.KeyExists(handle))
			return nullptr;

		FSDL3PlatformWindow* platformWindow = windowsByHandle[handle];
		if (!platformWindow)
			return nullptr;

		SDL_Window* sdlWindow = platformWindow->GetSdlHandle();

		SDL_PropertiesID props = SDL_GetWindowProperties(sdlWindow);

#if FUSION_PLATFORM_WINDOWS
		return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#else
		#error Platform Not Supported
#endif
	}

	void FSDL3PlatformBackend::PumpEvents()
	{
		// Handle SDL Events
		SDL_Event sdlEvent;

		while (SDL_PollEvent(&sdlEvent))
		{
			if (sdlEvent.type == SDL_EVENT_QUIT)
			{
				m_UserRequestedExit = true;

				for (const auto& [instanceHandle, instanceData] : instances)
				{
					if (instanceData.eventSink != nullptr)
					{
						instanceData.eventSink->OnExitRequested();
					}
				}
				break;
			}

			if (sdlEvent.type >= SDL_EVENT_WINDOW_FIRST && sdlEvent.type <= SDL_EVENT_WINDOW_LAST)
			{
				ProcessWindowEvents(sdlEvent);
			}

			if (sdlEvent.type == SDL_EVENT_KEY_DOWN || sdlEvent.type == SDL_EVENT_KEY_UP ||
				sdlEvent.type == SDL_EVENT_MOUSE_MOTION || sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN || sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_UP || sdlEvent.type == SDL_EVENT_MOUSE_WHEEL)
			{
				ProcessInputEvents(sdlEvent);
			}
		}
	}

	void FSDL3PlatformBackend::SetEventSink(FInstanceHandle instance, IFPlatformEventSink* eventSink)
	{
		if (!instances.KeyExists(instance))
		{
			return;
		}

		instances[instance].eventSink = eventSink;
	}

	FWindowHandle FSDL3PlatformBackend::CreateWindow(FInstanceHandle instance, const FString& title, u32 width, u32 height, const FPlatformWindowInfo& info)
	{
		FSDL3PlatformWindow* newWindow = new FSDL3PlatformWindow(title, width, height, info);
		
		if (newWindow->IsValid())
		{
			windowsByHandle[newWindow->GetWindowHandle()] = newWindow;
			return newWindow->GetWindowHandle();
		}
		
		delete newWindow;

		return FWindowHandle::NullValue;
	}

	void FSDL3PlatformBackend::DestroyWindow(FWindowHandle window)
	{
		if (!windowsByHandle.KeyExists(window))
		{
			return;
		}

		for (const auto& [instanceHandle, instanceData] : instances)
		{
			if (instanceData.eventSink != nullptr)
			{
				instanceData.eventSink->OnWindowDestroyed(window);
			}
		}

		FSDL3PlatformWindow* platformWindow = windowsByHandle[window];
		delete platformWindow;

		windowsByHandle.Remove(window);
	}

	void FSDL3PlatformBackend::ProcessWindowEvents(SDL_Event& event)
	{
		if (event.window.type == SDL_EVENT_WINDOW_RESIZED)
		{
			if (windowsByHandle.KeyExists(event.window.windowID))
			{
				ProcessWindowResizeEvent(windowsByHandle[event.window.windowID]);
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_MOVED)
		{
			for (const auto& [instanceHandle, instanceData] : instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowMoved(event.window.windowID, event.window.data1, event.window.data2);
				}
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) // Close a specific window
		{
			for (const auto& [instanceHandle, instanceData] : instances)
			{
				if (windowsByHandle.KeyExists(event.window.windowID))
				{
					FSDL3PlatformWindow* window = windowsByHandle[event.window.windowID];

					if (window)
					{
						if (FEnumHasFlag(window->GetInitialFlags(), FPlatformWindowFlags::DestroyOnClose))
						{
							DestroyWindow(event.window.windowID);
						}
						else
						{
							instanceData.eventSink->OnWindowClosed(event.window.windowID);
						}
					}
				}
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_MINIMIZED)
		{
			for (const auto& [instanceHandle, instanceData] : instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowMinimized(event.window.windowID);
				}
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_SHOWN)
		{
			for (const auto& [instanceHandle, instanceData] : instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowShown(event.window.windowID);
				}
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_RESTORED)
		{
			for (const auto& [instanceHandle, instanceData] : instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowRestored(event.window.windowID);
				}
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_MAXIMIZED)
		{
			for (const auto& [instanceHandle, instanceData] : instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowMaximized(event.window.windowID);
				}
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_DISPLAY_CHANGED)
		{
			for (const auto& [instanceHandle, instanceData] : instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					FDisplayId displayId = (FDisplayId)event.window.data1;

					instanceData.eventSink->OnWindowDisplayChanged(event.window.windowID, displayId);
				}
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
		{
			for (const auto& [instanceHandle, instanceData] : instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowKeyboardFocusChanged(event.window.windowID, true);
				}
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_FOCUS_LOST)
		{
			for (const auto& [instanceHandle, instanceData] : instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowKeyboardFocusChanged(event.window.windowID, true);
				}
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_MOUSE_ENTER)
		{
			for (const auto& [instanceHandle, instanceData] : instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowMouseFocusChanged(event.window.windowID, true);
				}
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_MOUSE_LEAVE)
		{
			for (const auto& [instanceHandle, instanceData] : instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowMouseFocusChanged(event.window.windowID, false);
				}
			}
		}
	}

	void FSDL3PlatformBackend::ProcessInputEvents(SDL_Event& event)
	{
		SDL_GetMouseState(&mousePosition.x, &mousePosition.y);
		u32 mouseBtnMask = SDL_GetGlobalMouseState(&globalMousePosition.x, &globalMousePosition.y);

		// TODO: Fix keyStatesDelayed: It is way too fast if FPS is high

		switch (event.type)
		{
		case SDL_EVENT_KEY_DOWN:
			inputWindowHandle = event.key.windowID;
			if (keyStates[(FKeyCode)event.key.key])
			{
				//keyStatesDelayed[(FKeyCode)event.key.key] = { .state = true, .lastEnabledTime = curTime };
			}
			else
			{
				stateChangesThisTick[(FKeyCode)event.key.key] = true;
			}
			keyStates[(FKeyCode)event.key.key] = true;
			modifierStates = (FKeyModifier)event.key.mod;
			break;
		case SDL_EVENT_KEY_UP:
			inputWindowHandle = event.key.windowID;
			if (keyStates[(FKeyCode)event.key.key])
			{
				stateChangesThisTick[(FKeyCode)event.key.key] = false;
			}
			keyStates[(FKeyCode)event.key.key] = false;
			//keyStatesDelayed[(FKeyCode)event.key.key] = { .state = false, .lastEnabledTime = 0 };
			modifierStates = (FKeyModifier)event.key.mod;
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			inputWindowHandle = event.button.windowID;
			if (mouseButtonStates[(FMouseButton)event.button.button] != event.button.clicks)
			{
				mouseButtonStateChanges[(FMouseButton)event.button.button] = event.button.clicks;
			}
			mouseButtonStates[(FMouseButton)event.button.button] = event.button.clicks;
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			inputWindowHandle = event.button.windowID;
			if (mouseButtonStates[(FMouseButton)event.button.button] != 0)
			{
				mouseButtonStateChanges[(FMouseButton)event.button.button] = 0;
			}
			mouseButtonStates[(FMouseButton)event.button.button] = 0;
			break;
		case SDL_EVENT_MOUSE_MOTION:
			inputWindowHandle = event.motion.windowID;
			break;
		case SDL_EVENT_MOUSE_WHEEL:
		{
			inputWindowHandle = event.wheel.windowID;
#if PLATFORM_MAC
			f32 flipX = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
			f32 flipY = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? 1 : -1;
#else
			f32 flipX = -1.0f, flipY = 1.0f;
#endif

			wheelDelta = FVec2(event.wheel.x * flipX, event.wheel.y * flipY);
		}
		break;
		}

		if (mouseButtonStates[FMouseButton::Left] != 0 && (mouseBtnMask & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) == 0)
		{
			mouseButtonStateChanges[FMouseButton::Left] = 0;
			mouseButtonStates[FMouseButton::Left] = 0;
		}
	}

	void FSDL3PlatformBackend::ProcessWindowResizeEvent(FSDL3PlatformWindow* window)
	{
		if (!window)
			return;

		int w = 0, h = 0;
		FVec2i size = window->GetDrawableWindowSize();
		w = size.x;
		h = size.y;

		if (w != 0 && h != 0)
		{
			for (const auto& [instanceHandle, instanceData] : instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowResized(window->GetWindowHandle(), w, h);
				}
			}
		}
	}

} // namespace Fusion

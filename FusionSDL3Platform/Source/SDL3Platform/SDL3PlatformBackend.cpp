#include "Fusion/SDL3Platform.h"

namespace Fusion
{
	bool FSDL3PlatformBackend::IsInitialized(FInstanceHandle instance)
	{
		return m_Instances.KeyExists(instance);
	}

	bool SDLEventWatch(void* userdata, SDL_Event* event)
	{
		FSDL3PlatformBackend* backend = (FSDL3PlatformBackend*)userdata;
		if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) 
		{
			FWindowHandle windowId = event->window.windowID;
			backend->ProcessWindowResizeEvent(windowId);
			backend->m_ContinuousResizeTick.ExecuteIfBound();
		}
		return true;
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

			SDL_AddEventWatch(SDLEventWatch, this);

			m_IsInitialized = true;
			m_UserRequestedExit = false;
		}

		if (m_InstanceCounter == FInstanceHandle::NullValue)
		{
			m_InstanceCounter = 0;
		}
		else
		{
			m_InstanceCounter = m_InstanceCounter.Get() + 1;
		}

		FInstanceHandle handle = m_InstanceCounter;

		m_Instances[handle] = FSDL3InstanceData{
			handle
		};

		return handle;
	}

	void FSDL3PlatformBackend::ShutdownInstance(FInstanceHandle instance)
	{
		if (!m_Instances.KeyExists(instance))
		{
			return;
		}

		m_Instances.Remove(instance);

		if (m_Instances.IsEmpty())
		{
			m_IsInitialized = false;

			SDL_RemoveEventWatch(SDLEventWatch, this);

			SDL_Quit();
		}
	}

	void FSDL3PlatformBackend::Tick()
	{
		auto now = std::chrono::high_resolution_clock::now();
		m_CurTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_StartTime).count();

		SDL_GetMouseState(&m_MousePosition.x, &m_MousePosition.y);
		SDL_GetGlobalMouseState(&m_GlobalMousePosition.x, &m_GlobalMousePosition.y);

		m_InputState.windowHandle = m_InputWindowHandle;
		m_InputState.stateChangesThisTick = m_StateChangesThisTick;
		m_InputState.keyStates = m_KeyStates;
		m_InputState.modifierStates = m_ModifierStates;
		//inputState.keyStatesDelayed = keyStatesDelayed;
		m_InputState.mouseButtonStates = m_MouseButtonStates;
		m_InputState.mouseButtonStateChanges = m_MouseButtonStateChanges;
		m_InputState.mousePosition = m_MousePosition;
		m_InputState.globalMousePosition = m_GlobalMousePosition;
		m_InputState.mouseDelta = m_GlobalMousePosition - m_PrevGlobalMousePosition;
		m_InputState.wheelDelta = m_WheelDelta;
		m_InputState.curTime = m_CurTime;

		// Reset temp values
		m_PrevGlobalMousePosition = m_GlobalMousePosition;
		m_WheelDelta = FVec2();

		m_StateChangesThisTick.Clear();
		m_MouseButtonStateChanges.Clear();
		m_FocusGainedWindows.Clear();
		m_FocusLostWindows.Clear();
	}

	void* FSDL3PlatformBackend::GetNativeWindowHandle(FWindowHandle handle)
	{
		if (!m_WindowsByHandle.KeyExists(handle))
			return nullptr;

		FSDL3PlatformWindow* platformWindow = m_WindowsByHandle[handle];
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

	void FSDL3PlatformBackend::SetContinuousResizeTick(const FDelegate<void()>& tick)
	{
		m_ContinuousResizeTick = tick;
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

				for (const auto& [instanceHandle, instanceData] : m_Instances)
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

	void FSDL3PlatformBackend::SetInstanceEventSink(FInstanceHandle instance, IFPlatformEventSink* eventSink)
	{
		if (!m_Instances.KeyExists(instance))
		{
			return;
		}

		m_Instances[instance].eventSink = eventSink;
	}

	void FSDL3PlatformBackend::RegisterEventSink(IFPlatformEventSink* eventSink)
	{
		m_RegisteredSinks.Add(eventSink);
	}

	void FSDL3PlatformBackend::DeregisterEventSink(IFPlatformEventSink* eventSink)
	{
		m_RegisteredSinks.Remove(eventSink);
	}


	FWindowHandle FSDL3PlatformBackend::CreateWindow(FInstanceHandle instance, const FString& title, u32 width, u32 height, const FPlatformWindowInfo& info)
	{
		FSDL3PlatformWindow* newWindow = new FSDL3PlatformWindow(title, width, height, info);
		
		if (newWindow->IsValid())
		{
			m_WindowsByHandle[newWindow->GetWindowHandle()] = newWindow;

			for (const auto& [instanceHandle, instanceData] : m_Instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowCreated(newWindow->GetWindowHandle());
				}
			}

			for (auto sink : m_RegisteredSinks)
			{
				sink->OnWindowCreated(newWindow->GetWindowHandle());
			}

			return newWindow->GetWindowHandle();
		}
		
		delete newWindow;

		return FWindowHandle::NullValue;
	}

	void FSDL3PlatformBackend::DestroyWindow(FWindowHandle window)
	{
		if (!m_WindowsByHandle.KeyExists(window))
		{
			return;
		}

		for (const auto& [instanceHandle, instanceData] : m_Instances)
		{
			if (instanceData.eventSink != nullptr)
			{
				instanceData.eventSink->OnWindowDestroyed(window);
			}
		}

		for (auto sink : m_RegisteredSinks)
		{
			sink->OnWindowDestroyed(window);
		}

		FSDL3PlatformWindow* platformWindow = m_WindowsByHandle[window];
		delete platformWindow;

		m_WindowsByHandle.Remove(window);
	}

	FVec2i FSDL3PlatformBackend::GetWindowSizeInPixels(FWindowHandle window)
	{
		if (!m_WindowsByHandle.KeyExists(window))
		{
			return {};
		}

		FSDL3PlatformWindow* sdlWindow = m_WindowsByHandle[window];
		if (!sdlWindow)
		{
			return {};
		}

		return sdlWindow->GetSizeInPixels();
	}

	void FSDL3PlatformBackend::ProcessWindowEvents(SDL_Event& event)
	{
		if (event.window.type == SDL_EVENT_WINDOW_RESIZED)
		{
			ProcessWindowResizeEvent(event.window.windowID);
		}
		else if (event.window.type == SDL_EVENT_WINDOW_MOVED)
		{
			for (const auto& [instanceHandle, instanceData] : m_Instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowMoved(event.window.windowID, event.window.data1, event.window.data2);
				}
			}

			for (auto sink : m_RegisteredSinks)
			{
				sink->OnWindowMoved(event.window.windowID, event.window.data1, event.window.data2);
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) // Close a specific window
		{
			FSDL3PlatformWindow* window = m_WindowsByHandle[event.window.windowID];
			if (window)
			{
				if (FEnumHasFlag(window->GetInitialFlags(), FPlatformWindowFlags::DestroyOnClose))
				{
					DestroyWindow(event.window.windowID);
				}
				else
				{
					for (const auto& [instanceHandle, instanceData] : m_Instances)
					{
						if (m_WindowsByHandle.KeyExists(event.window.windowID))
						{
							instanceData.eventSink->OnWindowClosed(event.window.windowID);
						}
					}

					for (auto sink : m_RegisteredSinks)
					{
						sink->OnWindowClosed(event.window.windowID);
					}
				}
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_MINIMIZED)
		{
			for (const auto& [instanceHandle, instanceData] : m_Instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowMinimized(event.window.windowID);
				}
			}

			for (auto sink : m_RegisteredSinks)
			{
				sink->OnWindowMinimized(event.window.windowID);
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_SHOWN)
		{
			for (const auto& [instanceHandle, instanceData] : m_Instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowShown(event.window.windowID);
				}
			}

			for (auto sink : m_RegisteredSinks)
			{
				sink->OnWindowShown(event.window.windowID);
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_RESTORED)
		{
			for (const auto& [instanceHandle, instanceData] : m_Instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowRestored(event.window.windowID);
				}
			}

			for (auto sink : m_RegisteredSinks)
			{
				sink->OnWindowRestored(event.window.windowID);
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_MAXIMIZED)
		{
			for (const auto& [instanceHandle, instanceData] : m_Instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowMaximized(event.window.windowID);
				}
			}

			for (auto sink : m_RegisteredSinks)
			{
				sink->OnWindowMaximized(event.window.windowID);
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_DISPLAY_CHANGED)
		{
			FDisplayId displayId = (FDisplayId)event.window.data1;

			for (const auto& [instanceHandle, instanceData] : m_Instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowDisplayChanged(event.window.windowID, displayId);
				}
			}

			for (auto sink : m_RegisteredSinks)
			{
				sink->OnWindowDisplayChanged(event.window.windowID, displayId);
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
		{
			for (const auto& [instanceHandle, instanceData] : m_Instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowKeyboardFocusChanged(event.window.windowID, true);
				}
			}

			for (auto sink : m_RegisteredSinks)
			{
				sink->OnWindowKeyboardFocusChanged(event.window.windowID, true);
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_FOCUS_LOST)
		{
			for (const auto& [instanceHandle, instanceData] : m_Instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowKeyboardFocusChanged(event.window.windowID, false);
				}
			}

			for (auto sink : m_RegisteredSinks)
			{
				sink->OnWindowKeyboardFocusChanged(event.window.windowID, false);
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_MOUSE_ENTER)
		{
			for (const auto& [instanceHandle, instanceData] : m_Instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowMouseFocusChanged(event.window.windowID, true);
				}
			}

			for (auto sink : m_RegisteredSinks)
			{
				sink->OnWindowMouseFocusChanged(event.window.windowID, true);
			}
		}
		else if (event.window.type == SDL_EVENT_WINDOW_MOUSE_LEAVE)
		{
			for (const auto& [instanceHandle, instanceData] : m_Instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowMouseFocusChanged(event.window.windowID, false);
				}
			}

			for (auto sink : m_RegisteredSinks)
			{
				sink->OnWindowMouseFocusChanged(event.window.windowID, false);
			}
		}
	}

	void FSDL3PlatformBackend::ProcessInputEvents(SDL_Event& event)
	{
		SDL_GetMouseState(&m_MousePosition.x, &m_MousePosition.y);
		u32 mouseBtnMask = SDL_GetGlobalMouseState(&m_GlobalMousePosition.x, &m_GlobalMousePosition.y);

		// TODO: Fix keyStatesDelayed: It is way too fast if FPS is high

		switch (event.type)
		{
		case SDL_EVENT_KEY_DOWN:
			m_InputWindowHandle = event.key.windowID;
			if (m_KeyStates[(FKeyCode)event.key.key])
			{
				//keyStatesDelayed[(FKeyCode)event.key.key] = { .state = true, .lastEnabledTime = curTime };
			}
			else
			{
				m_StateChangesThisTick[(FKeyCode)event.key.key] = true;
			}
			m_KeyStates[(FKeyCode)event.key.key] = true;
			m_ModifierStates = (FKeyModifier)event.key.mod;
			break;
		case SDL_EVENT_KEY_UP:
			m_InputWindowHandle = event.key.windowID;
			if (m_KeyStates[(FKeyCode)event.key.key])
			{
				m_StateChangesThisTick[(FKeyCode)event.key.key] = false;
			}
			m_KeyStates[(FKeyCode)event.key.key] = false;
			//keyStatesDelayed[(FKeyCode)event.key.key] = { .state = false, .lastEnabledTime = 0 };
			m_ModifierStates = (FKeyModifier)event.key.mod;
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			m_InputWindowHandle = event.button.windowID;
			if (m_MouseButtonStates[(FMouseButton)event.button.button] != event.button.clicks)
			{
				m_MouseButtonStateChanges[(FMouseButton)event.button.button] = event.button.clicks;
			}
			m_MouseButtonStates[(FMouseButton)event.button.button] = event.button.clicks;
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			m_InputWindowHandle = event.button.windowID;
			if (m_MouseButtonStates[(FMouseButton)event.button.button] != 0)
			{
				m_MouseButtonStateChanges[(FMouseButton)event.button.button] = 0;
			}
			m_MouseButtonStates[(FMouseButton)event.button.button] = 0;
			break;
		case SDL_EVENT_MOUSE_MOTION:
			m_InputWindowHandle = event.motion.windowID;
			break;
		case SDL_EVENT_MOUSE_WHEEL:
		{
			m_InputWindowHandle = event.wheel.windowID;
#if PLATFORM_MAC
			f32 flipX = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
			f32 flipY = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? 1 : -1;
#else
			f32 flipX = -1.0f, flipY = 1.0f;
#endif

			m_WheelDelta = FVec2(event.wheel.x * flipX, event.wheel.y * flipY);
		}
		break;
		}

		if (m_MouseButtonStates[FMouseButton::Left] != 0 && (mouseBtnMask & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) == 0)
		{
			m_MouseButtonStateChanges[FMouseButton::Left] = 0;
			m_MouseButtonStates[FMouseButton::Left] = 0;
		}
	}

	void FSDL3PlatformBackend::ProcessWindowResizeEvent(FWindowHandle windowHandle)
	{
		if (windowHandle.IsNull())
			return;
		if (!m_WindowsByHandle.KeyExists(windowHandle))
			return;

		FSDL3PlatformWindow* window = m_WindowsByHandle[windowHandle];
		if (!window)
			return;

		int w = 0, h = 0;
		FVec2i size = window->GetSizeInPixels();
		w = size.x;
		h = size.y;

		if (w != 0 && h != 0)
		{
			for (const auto& [instanceHandle, instanceData] : m_Instances)
			{
				if (instanceData.eventSink != nullptr)
				{
					instanceData.eventSink->OnWindowResized(window->GetWindowHandle(), w, h);
				}
			}

			for (auto sink : m_RegisteredSinks)
			{
				sink->OnWindowResized(window->GetWindowHandle(), w, h);
			}
		}
	}

} // namespace Fusion

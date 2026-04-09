#include "Fusion/SDL3Platform.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

	FSDL3PlatformWindow::FSDL3PlatformWindow(const FString& title, u32 width, u32 height, const FPlatformWindowInfo& info)
	{
		u32 flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
		if (info.resizable)
			flags |= SDL_WINDOW_RESIZABLE;
		if (info.hidden)
			flags |= SDL_WINDOW_HIDDEN;
		if (info.borderless)
			flags |= SDL_WINDOW_BORDERLESS;

#if FUSION_TRAIT_METAL_SUPPORTED
		flags |= SDL_WINDOW_METAL;
#elif FUSION_TRAIT_VULKAN_SUPPORTED
		flags |= SDL_WINDOW_VULKAN;
#endif

		if (info.maximised)
			flags |= SDL_WINDOW_MAXIMIZED;
		if (info.fullscreen)
			flags |= SDL_WINDOW_FULLSCREEN;
		if (FEnumHasFlag(info.windowFlags, FPlatformWindowFlags::PopupMenu))
			flags |= SDL_WINDOW_POPUP_MENU;
		if (FEnumHasFlag(info.windowFlags, FPlatformWindowFlags::ToolTip))
			flags |= SDL_WINDOW_TOOLTIP;
		if (FEnumHasFlag(info.windowFlags, FPlatformWindowFlags::Utility))
			flags |= SDL_WINDOW_UTILITY;

		int x = SDL_WINDOWPOS_CENTERED;
		int y = SDL_WINDOWPOS_CENTERED;

		if (!info.openCentered)
		{
			x = info.openPos.x;
			y = info.openPos.y;
		}

		sdlWindow = SDL_CreateWindow(title.CStr(), width, height, flags);
		if (sdlWindow == nullptr)
		{
			FUSION_LOG_ERROR("SDL3PlatformWindow", "Failed to create SDL window: {}", SDL_GetError());
			return;
		}

		initialFlags = info.windowFlags;
		
		SDL_SetWindowPosition(sdlWindow, x, y);
		SDL_SetWindowMinimumSize(sdlWindow, 32, 32);
		
		windowHandle = SDL_GetWindowID(sdlWindow);
	}

	FSDL3PlatformWindow::~FSDL3PlatformWindow()
	{
		if (sdlWindow)
		{
			SDL_DestroyWindow(sdlWindow);
			sdlWindow = nullptr;
		}
	}

	FVec2i FSDL3PlatformWindow::GetSizeInPixels()
	{
		int w = 0, h = 0;
		SDL_GetWindowSizeInPixels(sdlWindow, &w, &h);
		return FVec2i(w, h);
	}

	FVec2i FSDL3PlatformWindow::GetSize()
	{
		int w = 0, h = 0;
		SDL_GetWindowSize(sdlWindow, &w, &h);
		return FVec2i(w, h);
	}

	FVec2i FSDL3PlatformWindow::GetPosition()
	{
		int x = 0, y = 0;
		SDL_GetWindowPosition(sdlWindow, &x, &y);
		return FVec2i(x, y);
	}

	f32 FSDL3PlatformWindow::GetDpiScale()
	{
		return SDL_GetWindowDisplayScale(sdlWindow);
	}
} // namespace Fusion


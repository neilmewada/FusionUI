#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#include "../SDL3Platform.h"

namespace Fusion
{
    class FSDL3WindowsPlatform : public FSDL3PlatformBase
    {
    public:
        FSDL3WindowsPlatform() = delete;

        static f32 GetWindowDpiScale(SDL_Window* sdlWindow)
        {
            return SDL_GetWindowDisplayScale(sdlWindow);
        }

        static void SetupWindow([[maybe_unused]] SDL_Window* sdlWindow, [[maybe_unused]] const FPlatformWindowInfo& info) {}
    };

    typedef FSDL3WindowsPlatform FSDL3Platform;

} // namespace Fusion

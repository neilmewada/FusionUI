#pragma once

namespace Fusion {
    class FSDL3LinuxPlatform : public FSDL3PlatformBase
    {
    public:
        FSDL3LinuxPlatform() = delete;

        static f32 GetWindowDpiScale(SDL_Window* sdlWindow)
        {
            return SDL_GetWindowDisplayScale(sdlWindow);
        }

        static void SetupWindow([[maybe_unused]] SDL_Window* sdlWindow, [[maybe_unused]] const FPlatformWindowInfo& info) {}
    };

    typedef FSDL3LinuxPlatform FSDL3Platform;
}
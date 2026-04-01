#pragma once

namespace Fusion
{

	class FUSIONSDL3PLATFORM_API FSDL3PlatformWindow
    {
    public:

        FSDL3PlatformWindow(const FString& title, u32 width, u32 height, const FPlatformWindowInfo& info);

		~FSDL3PlatformWindow();

		bool IsValid() const { return windowHandle != FWindowHandle::NullValue; }

		FWindowHandle GetWindowHandle() const { return windowHandle; }

		FVec2i GetDrawableWindowSize();

		FPlatformWindowFlags GetInitialFlags() const { return initialFlags; }

		SDL_Window* GetSdlHandle() const { return sdlWindow; }

    private:

		FWindowHandle windowHandle = FWindowHandle::NullValue;
		SDL_Window* sdlWindow = nullptr;

		FPlatformWindowFlags initialFlags = FPlatformWindowFlags::None;
    };

} // namespace Fusion

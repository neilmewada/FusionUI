#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    enum class FPlatformWindowFlags
    {
        None = 0,
        ToolTip = FUSION_BIT(0),
        PopupMenu = FUSION_BIT(1),
        Utility = FUSION_BIT(2),
        DestroyOnClose = FUSION_BIT(3)
    };
    FUSION_ENUM_CLASS_FLAGS(FPlatformWindowFlags);

    struct FPlatformWindowInfo
    {
        bool maximised = false;
        bool fullscreen = false;
        bool resizable = true;
        bool hidden = false;
		bool borderless = false;

		FDisplayId displayId = FDisplayId::NullValue;
        bool openCentered = true;
        FVec2i openPos = FVec2i();
        FPlatformWindowFlags windowFlags = FPlatformWindowFlags::DestroyOnClose;
    };

    class FUSIONPLATFORM_API IFPlatformBackend
	{
    public:

        virtual ~IFPlatformBackend() = default;

		// - Lifecycle -

		virtual bool IsInitialized(FInstanceHandle instance) = 0;

        virtual FInstanceHandle InitializeInstance() = 0;

        virtual void Tick() = 0;

        virtual void ShutdownInstance(FInstanceHandle instance) = 0;

		virtual bool IsUserRequestingExit() = 0;

        // - Native Handles -

        virtual void* GetNativeWindowHandle(FWindowHandle handle) = 0;

		// - Events -

        virtual void PumpEvents() = 0;

		virtual void SetEventSink(FInstanceHandle instance, IFPlatformEventSink* eventSink) = 0;

		// - Window Management -

		virtual FWindowHandle CreateWindow(FInstanceHandle instance, const FString& title, u32 width, u32 height, const FPlatformWindowInfo& info) = 0;

		virtual void DestroyWindow(FWindowHandle window) = 0;

	};
    
} // namespace Fusion

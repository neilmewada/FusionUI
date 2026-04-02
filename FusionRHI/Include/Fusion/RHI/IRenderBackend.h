#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	using FResourceHandle = Handle<u32>;

	struct FUIVertex
	{
		FVec2 pos;
		FVec2 uv;
		u32 color = 0;
		u32 drawItemIndex = 0;
	};

	enum class FGraphicsBackendType
	{
		Unknown = 0,
		Vulkan,
		Metal
	};
	FUSION_ENUM_CLASS(FGraphicsBackendType);

	struct FRenderCapabilities
	{
		
	};
    
    class FUSIONRHI_API IFRenderBackend : public IFPlatformEventSink
    {
    public:

		IFRenderBackend(IFPlatformBackend* platformBackend) : m_PlatformBackend(platformBackend)
		{
			FUSION_ASSERT(platformBackend != nullptr, "RenderBackend constructed without a platformBackend reference.");
		}

		virtual ~IFRenderBackend() = default;

		IFPlatformBackend* GetPlatformBackend() const { return m_PlatformBackend; }

		// - Capabilities -

		virtual FGraphicsBackendType GetGraphicsBackendType() { return FGraphicsBackendType::Unknown; }

		virtual FRenderCapabilities GetRenderCapabilities() = 0;

		// - Lifecycle -

		virtual bool IsInitialized(FInstanceHandle instance) = 0;

		virtual bool InitializeInstance(FInstanceHandle instance) = 0;

		virtual void ShutdownInstance(FInstanceHandle instance) = 0;

		// - Rendering -

		// Render loop, only called when using FApplication.Run()
		virtual void RenderTick() = 0;

		// TODO: Need to come up with a better API
		virtual void SubmitFrame(FInstanceHandle instance) {}

    protected:

		IFPlatformBackend* m_PlatformBackend = nullptr;
    };

} // namespace Fusion

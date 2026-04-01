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
    
    class FUSIONRHI_API IFRenderBackend
    {
    public:

		IFRenderBackend(IFPlatformBackend* platformBackend) : m_PlatformBackend(platformBackend)
		{
			
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

		// TODO: Need to come up with a better API
		virtual void SubmitFrame(FInstanceHandle instance) {}

    protected:

		IFPlatformBackend* m_PlatformBackend = nullptr;
    };

} // namespace Fusion

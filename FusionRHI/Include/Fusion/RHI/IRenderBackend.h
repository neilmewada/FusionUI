#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	using FResourceHandle = FHandle<u32>;

	FUSION_DEFINE_HANDLE_TYPE(FAtlasHandle, u32);

	enum class ERenderTargetType
	{
		Window = 0,
	};
	FUSION_ENUM_CLASS(ERenderTargetType);

	enum class EGraphicsBackendType
	{
		None = 0,
		Vulkan,
		Metal,
		Other,
	};
	FUSION_ENUM_CLASS(EGraphicsBackendType);

	struct FRenderBackendCapabilities
	{
		SizeT MinConstantBufferOffsetAlignment = 0x40;
		SizeT MinStructuredBufferOffsetAlignment = 0x10;
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

		virtual EGraphicsBackendType GetGraphicsBackendType() { return EGraphicsBackendType::None; }

		virtual FRenderBackendCapabilities GetRenderCapabilities() = 0;

		// - Lifecycle -

		virtual bool IsInitialized(FInstanceHandle instance) = 0;

		virtual bool InitializeInstance(FInstanceHandle instance) = 0;

		virtual void ShutdownInstance(FInstanceHandle instance) = 0;

		// - Atlas -

		virtual FAtlasHandle CreateLayeredAtlas(bool grayscale, u32 resolution, u32 maxLayers) = 0;

    	virtual void UploadAtlasRegionAsync(FAtlasHandle atlas, u32 layer,
    		FVec2i pos, FVec2i size,
    		const u8* pixels, int pitch) = 0;

		virtual void SetFontAtlas(FInstanceHandle instance, FAtlasHandle atlas) = 0;

    	virtual void DestroyAtlas(FAtlasHandle atlas) = 0;

		// - Rendering -

		virtual FRenderTargetHandle AcquireWindowRenderTarget(FInstanceHandle instance, FWindowHandle window) = 0;
		virtual void ReleaseRenderTarget(FInstanceHandle instance, FRenderTargetHandle renderTarget) = 0;

		// Render loop, only called when using FApplication.Run()
		virtual void RenderTick() = 0;

		virtual void SubmitSnapshot(FRenderTargetHandle renderTarget, IntrusivePtr<FRenderSnapshot> snapshot) = 0;

    protected:

		IFPlatformBackend* m_PlatformBackend = nullptr;
    };

} // namespace Fusion

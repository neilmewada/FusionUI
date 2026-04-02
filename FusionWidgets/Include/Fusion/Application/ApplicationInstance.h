#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	class FWidget;
	class FSurface;

    struct FApplicationInstanceDesc
    {
		IFPlatformBackend* platformBackend = nullptr;
		IFRenderBackend* renderBackend = nullptr;
	};

	class FUSIONWIDGETS_API FApplicationInstance : public FObject, public IFPlatformEventSink
    {
        FUSION_CLASS(FApplicationInstance, FObject)
    public:

        FApplicationInstance(FName name) : FObject(name)
        {
	        
        }

		bool Initialize(const FApplicationInstanceDesc& desc);

		void Shutdown();

		FInstanceHandle GetInstanceHandle() const { return m_InstanceHandle; }

	protected:

		void OnWindowDestroyed(FWindowHandle window) override;

		FInstanceHandle m_InstanceHandle = FInstanceHandle::NullValue;

		FArray<Ptr<FSurface>> m_Surfaces;

    private:

		bool m_PlatformBackendAllocated = false;
		IFPlatformBackend* m_PlatformBackend = nullptr;

		bool m_RenderBackendAllocated = false;
		IFRenderBackend* m_RenderBackend = nullptr;
    };
    
} // namespace Fusion

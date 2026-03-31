#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	using FResourceHandle = Handle<u32>;

	struct FRenderCapabilities
	{
		
	};
    
    class FUSIONRHI_API IFRenderBackend
    {
    public:

		virtual ~IFRenderBackend() = default;

		// - Capabilities -

		virtual FRenderCapabilities GetRenderCapabilities() = 0;

		// - Lifecycle -

		virtual bool IsInitialized(FInstanceHandle instance) = 0;

		virtual bool InitializeInstance(FInstanceHandle instance) = 0;

		virtual void ShutdownInstance(FInstanceHandle instance) = 0;

    };

} // namespace Fusion

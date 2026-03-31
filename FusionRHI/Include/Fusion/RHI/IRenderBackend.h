#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    
    class FUSIONRHI_API IFRenderBackend
    {
    public:

		virtual ~IFRenderBackend() = default;

		// - Lifecycle -

		virtual void InitializeShaders() = 0;

		virtual bool IsInitialized(FInstanceHandle instance) = 0;

		virtual bool InitializeInstance(FInstanceHandle instance) = 0;

		virtual void ShutdownInstance(FInstanceHandle instance) = 0;

    };

} // namespace Fusion

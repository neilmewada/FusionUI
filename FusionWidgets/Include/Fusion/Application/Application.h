#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    class FUSIONWIDGETS_API FApplication
    {
    public:

		FApplication(int argc, char** argv);

		void SetPlatformBackend(IFPlatformBackend* platformBackend) { m_PlatformBackend = platformBackend; }

		void SetRenderBackend(IFRenderBackend* renderBackend) { m_RenderBackend = renderBackend; }

        Ptr<FApplicationInstance> GetMainApplication() const { return m_MainApplication; }

        int Run();

    private:

		Ptr<FApplicationInstance> m_MainApplication;

		IFRenderBackend* m_RenderBackend = nullptr;
		IFPlatformBackend* m_PlatformBackend = nullptr;

    };
    
} // namespace Fusion

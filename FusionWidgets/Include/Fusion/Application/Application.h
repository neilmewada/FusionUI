#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FWidget;
    class FTheme;

    class FUSIONWIDGETS_API FApplication
    {
    public:

		FApplication(int argc, char** argv);

		void SetPlatformBackend(IFPlatformBackend* platformBackend) { m_PlatformBackend = platformBackend; }

		void SetRenderBackend(IFRenderBackend* renderBackend) { m_RenderBackend = renderBackend; }

        Ref<FApplicationInstance> GetMainApplication() const { return m_MainApplication; }

        template<class TWidget, class... TArgs>
        TWidget& CreateMainWindow(TArgs&&... args) requires TFIsDerivedClass<FWidget, TWidget>::Value
		{
            Ref<TWidget> ptr = NewObject<TWidget>(nullptr, std::forward<TArgs>(args)...);
            m_MainWindow = ptr;
            return *ptr;
		}

        Ref<FTheme> CreateDefaultStyleSheet();

        int Run();

    private:

        Ref<FWidget> m_MainWindow;
        FVec2i m_InitialWindowSize = FVec2i(1200, 900);

		Ref<FApplicationInstance> m_MainApplication;
        Ref<FTheme> m_MainStyleSheet;

		IFRenderBackend* m_RenderBackend = nullptr;
		IFPlatformBackend* m_PlatformBackend = nullptr;

    };
    
} // namespace Fusion

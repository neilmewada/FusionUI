#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FTreeViewHeader;
    class FTreeViewContent;

    class FUSIONWIDGETS_API FTreeView : public FItemView
    {
        FUSION_WIDGET(FTreeView, FItemView)
    protected:

        FTreeView();

        void Construct() override;

    public:

    protected:

        void OnModelChanged() override;

        Ref<FVerticalStack> m_Container;
        Ref<FTreeViewHeader> m_Header;
        Ref<FScrollBox> m_ScrollBox;
        Ref<FTreeViewContent> m_Content;

    public:
        // - Fusion Properties -

        
    };
    
} // namespace Fusion

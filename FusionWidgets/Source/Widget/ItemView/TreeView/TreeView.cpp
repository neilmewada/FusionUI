#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    FTreeView::FTreeView()
    {

    }

    void FTreeView::Construct()
    {
        Super::Construct();

        StyleScopeBoundary(true);

        Child(
            FAssignNew(FVerticalStack, m_Container)
            .ContentHAlign(EHAlign::Fill)
            .ContentVAlign(EVAlign::Top)
            .HAlign(EHAlign::Fill)
            .VAlign(EVAlign::Fill)
            (
                FAssignNew(FTreeViewHeader, m_Header)
                .SubStyle("Header"),

                FAssignNew(FScrollBox, m_ScrollBox)
                .CanScrollHorizontal(true)
                .CanScrollVertical(true)
                .HorizontalScrollVisibility(EScrollbarVisibility::Auto)
                .VerticalScrollVisibility(EScrollbarVisibility::Auto)
                .SubStyle("ScrollBox")
                .Child(
                    FAssignNew(FTreeViewContent, m_Content)
                    .TreeView(this)
                    .SubStyle("Content")
                )
            )
        );
    }

    void FTreeView::OnModelChanged()
    {
        Super::OnModelChanged();

        
    }

} // namespace Fusion

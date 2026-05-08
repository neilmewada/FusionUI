#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    FTreeView::FTreeView()
    {
        m_RowHeight = 24.0f;
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
            .Padding(FMargin())
            (
                FAssignNew(FTreeViewHeader, m_Header)
                .SubStyle("Header"),

                FAssignNew(FScrollBox, m_ScrollBox)
                .CanScrollHorizontal(false)
                .CanScrollVertical(true)
                .HorizontalScrollVisibility(EScrollbarVisibility::Auto)
                .VerticalScrollVisibility(EScrollbarVisibility::Auto)
                .ScrollSpeed(15.0f)
                .SubStyle("ScrollBox")
                .Child(
                    FAssignNew(FTreeViewContent, m_Content)
                    .TreeView(this)
                    .SubStyle("Content")
                    .Margin(FMargin())
                )
                .OnScrollOffsetChanged([this](FVec2 offset)
                {
                    m_Content->OnScrollOffsetChanged(offset);
                })
                .FillRatio(1.0f)
            )
        );
    }

    bool FTreeView::IsExpanded(FModelIndex index)
    {
        return m_Content->IsExpanded(index);
    }

    void FTreeView::OnModelChanged()
    {
        Super::OnModelChanged();

        Ref<FItemModel> model = Model();

        if (Model())
        {
            if (model->HasHeader())
            {
                m_Header->Excluded(false);
                m_Header->UpdateHeaderData(model);
            }
            else
            {
                m_Header->Excluded(true);
            }

            m_Content->OnModelChanged(model);
        }
    }

    void FTreeView::OnSelectionChanged(const FModelIndexList& added, const FModelIndexList& removed)
    {
        Super::OnSelectionChanged(added, removed);

        m_Content->UpdateSelection();
    }

} // namespace Fusion

#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    FTreeViewRow::FTreeViewRow()
    {

    }

    void FTreeViewRow::Construct()
    {
        Super::Construct();

        Child(
            FAssignNew(FHorizontalStack, m_HStack)
            .ContentVAlign(EVAlign::Fill)
            .HAlign(EHAlign::Fill)
            .VAlign(EVAlign::Fill)
        );
    }

    void FTreeViewRow::Paint(FPainter& painter)
    {
        Super::Paint(painter);

        Ref<FTreeView> treeView = GetTreeView();
        if (!treeView || m_Columns.Empty())
            return;

        Ref<FItemModel> model = treeView->Model();
        if (!model)
            return;

        Ref<FItemViewDelegate> delegate = treeView->ItemDelegate();
        if (!delegate)
            return;

        const FVec2 layoutSize = GetLayoutSize();

        // Column boundaries in header-local space: [0, col1_start, col2_start, ..., headerWidth]
        // Using boundaries instead of child widths gives pixel-accurate alignment with the
        // visual splitter positions regardless of label margins or fill-ratio rounding.
        TArray<f32> boundaries;

        if (Ref<FTreeViewHeader> header = treeView->GetHeader())
        {
            boundaries   = header->GetColumnBoundaries();

            // The row is narrower than the header by the scrollbar width.
            // Clamp the last boundary to the row width so the last column
            // doesn't overflow, while all earlier boundaries stay pixel-perfect.
            if (!boundaries.Empty())
                boundaries.Last() = FMath::Min(boundaries.Last(), layoutSize.width);
        }
        else
        {
            // No header — divide equally by fill ratio hints
            boundaries.Add(0.0f);
            f32 totalFillRatio = 0.0f;
            for (int i = 0; i < m_Columns.Size(); i++)
                totalFillRatio += model->GetColumnFillRatioHint(i);

            f32 cursor = 0.0f;
            for (int i = 0; i < m_Columns.Size(); i++)
            {
                f32 ratio = totalFillRatio > 0.001f
                    ? model->GetColumnFillRatioHint(i) / totalFillRatio
                    : 1.0f / (f32)m_Columns.Size();
                cursor += ratio * layoutSize.width;
                boundaries.Add(cursor);
            }
        }

        const f32 colY        = Padding().top;
        const f32 colH        = layoutSize.y - Padding().top - Padding().bottom;
        const f32 depthIndent = Padding().left;  // set by UpdateVisibleRows: depth * indentWidth

        for (int i = 0; i < m_Columns.Size(); ++i)
        {
            if (i + 1 >= boundaries.Size())
                break;

            const f32 colStart  = boundaries[i] + (i == 0 ? depthIndent : 0.0f);
            const f32 colEnd    = boundaries[i + 1];

            FItemViewPaintInfo paintInfo{};
            paintInfo.Rect  = FRect(FVec2(colStart, colY), FVec2(colEnd, colY + colH));
            paintInfo.Model = model;
            paintInfo.View  = treeView;

            delegate->Paint(painter, m_Columns[i], paintInfo);
        }
    }

    void FTreeViewRow::SetData(const TArray<FModelIndex>& columns)
    {
        m_Columns = columns;
    }

    void FTreeViewRow::OnMouseEnter(FMouseEvent& event)
    {
        Super::OnMouseEnter(event);

        SetStyleStateFlag(EStyleState::Hovered, true);
    }

    FEventReply FTreeViewRow::OnMouseMove(FMouseEvent& event)
    {
        return Super::OnMouseMove(event);
    }

    void FTreeViewRow::OnMouseLeave(FMouseEvent& event)
    {
        Super::OnMouseLeave(event);

        SetStyleStateFlag(EStyleState::Hovered, false);
    }

    FEventReply FTreeViewRow::OnMouseButtonDown(FMouseEvent& event)
    {
        if (event.IsLeftButton())
        {
            if (event.ClickCount == 2)
            {
                if (Ref<FTreeView> treeView = GetTreeView())
                {
                    treeView->GetContent()->ToggleExpanded(m_RowIndex);
                }
            }
            return FEventReply::Handled();
        }
        return Super::OnMouseButtonDown(event);
    }
} // namespace Fusion

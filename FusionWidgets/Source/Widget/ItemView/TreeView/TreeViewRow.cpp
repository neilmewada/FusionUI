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
        ZoneScoped;

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
        const f32 depthIndent = Padding().left;

        // --- Tree connector lines ---
        if (const FPen treeLinePen = treeView->TreeLinePen(); treeLinePen.IsValid() && m_Depth > 0)
        {
            const f32 indentWidth = treeView->RowIndentWidth();
            const f32 rowHeight   = layoutSize.y;
            const f32 centerY     = rowHeight * 0.5f;

            painter.SetPen(treeLinePen);

            // Vertical continuation bars for all ancestor depths except the direct parent
            for (int d = 0; d < m_Depth - 1; d++)
            {
                if (m_ContinuationMask & (1ULL << d))
                {
                    const f32 x = (f32)d * indentWidth + indentWidth * 0.5f;
                    painter.DrawLine(FVec2(x, 0.0f), FVec2(x, rowHeight));
                }
            }

            // Elbow at the direct parent's indent position (depth - 1)
            const f32 elbowX  = (f32)(m_Depth - 1) * indentWidth + indentWidth * 0.5f;
            const bool isLast = !(m_ContinuationMask & (1ULL << m_Depth));

            painter.DrawLine(FVec2(elbowX, 0.0f),    FVec2(elbowX, centerY));     // top half always
            if (!isLast)
                painter.DrawLine(FVec2(elbowX, centerY), FVec2(elbowX, rowHeight)); // bottom half if ├

            // Horizontal arm from elbow to the start of content
            const f32 contentX = (f32)m_Depth * indentWidth;
            painter.DrawLine(FVec2(elbowX, centerY), FVec2(contentX, centerY));
        }

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
            if (Ref<FTreeView> treeView = GetTreeView())
            {
                if (event.ClickCount == 2)
                {
                    // Double-click: toggle expand/collapse — O(1) via flat index
                    treeView->GetContent()->ToggleExpanded(m_FlatRowIndex);
                }
            }
            return FEventReply::Handled();
        }
        return Super::OnMouseButtonDown(event);
    }
} // namespace Fusion

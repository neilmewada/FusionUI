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

        const FVec2 layoutSize = GetLayoutSize();

        TArray<f32> childWidths;
        f32 splitterSpacing = 0;
        if (Ref<FTreeViewHeader> header = treeView->GetHeader())
        {
            childWidths     = header->GetChildrenWidths();
            splitterSpacing = header->GetSplitterSpacing();

            // The row is narrower than the header by a constant amount
            // (scrollbar width + content padding). Keep all column boundaries
            // at exactly the same pixel positions as the header splitters —
            // just absorb the difference into the last column's right edge.
            f32 totalHeaderWidth = splitterSpacing * (f32)childWidths.Size();
            for (f32 w : childWidths)
                totalHeaderWidth += w;

            const f32 widthDiff = totalHeaderWidth - layoutSize.width;
            if (!childWidths.Empty() && widthDiff > 0.001f)
                childWidths.Last() -= widthDiff;
        }
        else
        {
            childWidths.Resize(m_Columns.Size());
            f32 totalFillRatio = 0.0f;

            for (int i = 0; i < m_Columns.Size(); i++)
            {
                childWidths[i] = model->GetColumnFillRatioHint(i);
                totalFillRatio += childWidths[i];
            }

            for (int i = 0; i < m_Columns.Size(); i++)
            {
                if (totalFillRatio > TNumericLimits<f32>::Epsilon())
                {
                    childWidths[i] /= totalFillRatio;
                }
                else
                {
                    childWidths[i] = (i + 1.0f) / m_Columns.Size();
                }

                childWidths[i] *= layoutSize.width;
            }
        }

        Ref<FItemViewDelegate> delegate = treeView->ItemDelegate();
        if (!delegate)
            return;

        f32 offsetX = 0;

        for (int i = 0; i < m_Columns.Size(); ++i)
        {
            FModelIndex index = m_Columns[i];

            FItemViewPaintInfo paintInfo{};

            f32 indent = (i == 0) ? Padding().left : 0.0f;
            f32 colX   = offsetX + indent;
            f32 colW   = childWidths[i] - indent;
            f32 colY   = Padding().top;
            f32 colH   = layoutSize.y - Padding().top - Padding().bottom;

            paintInfo.Rect = FRect::FromSize(FVec2(colX, colY), FVec2(colW, colH));
            paintInfo.Model = model;

            delegate->Paint(painter, index, paintInfo);

            offsetX += childWidths[i] + splitterSpacing;
        }
    }

    void FTreeViewRow::SetData(const TArray<FModelIndex>& columns)
    {
        m_Columns = columns;
    }

} // namespace Fusion

#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    FTreeViewContent::FTreeViewContent()
    {

    }

    void FTreeViewContent::Construct()
    {
        Super::Construct();


    }

    void FTreeViewContent::OnModelChanged(Ref<FItemModel> model)
    {
        if (!model)
            return;

        RebuildFlatRows();
        MarkLayoutDirty();
        MarkPaintDirty();
    }

    void FTreeViewContent::OnScrollOffsetChanged(FVec2 offset)
    {
        UpdateVisibleRows(m_CachedFinalSize);
    }

    void FTreeViewContent::RebuildFlatRows()
    {
        m_FlatRows.Clear();
        AppendRows({}, 0);  // start from root
    }

    void FTreeViewContent::AppendRows(FModelIndex parent, int depth)
    {
        Ref<FTreeView> treeView = GetTreeView();
        if (!treeView)
            return;

        Ref<FItemModel> model = treeView->Model();
        if (!model)
            return;

        u32 count = model->GetRowCount(parent);
        for (u32 i = 0; i < count; i++)
        {
            FModelIndex index = model->GetIndex(i, 0, parent);
            u32 childCount    = model->GetRowCount(index);
            m_FlatRows.Add({ index, parent, depth, childCount > 0 });

            if (childCount > 0 && m_ExpandedItems.Contains(index))
                AppendRows(index, depth + 1);
        }
    }

    FVec2 FTreeViewContent::MeasureContent(FVec2 availableSize)
    {
        Ref<FTreeView> treeView = GetTreeView();
        if (!treeView || !treeView->Model())
            return m_DesiredSize = ApplyLayoutConstraints(availableSize);

        Ref<FItemModel> model = treeView->Model();

        return m_DesiredSize = ApplyLayoutConstraints(FVec2(
            availableSize.x,
            m_FlatRows.Size() * treeView->RowHeight()
        ));
    }

    void FTreeViewContent::ArrangeContent(FVec2 finalSize)
    {
        ArrangeContentBase(finalSize);

        Ref<FTreeView> treeView = GetTreeView();
        if (!treeView)
            return;

        m_CachedFinalSize = finalSize;
        UpdateVisibleRows(finalSize);
    }

    void FTreeViewContent::ToggleExpanded(FModelIndex index)
    {
        if (!index.IsValid())
            return;

        // Find the flat index of this node
        int flatIdx = -1;
        for (int i = 0; i < (int)m_FlatRows.Size(); i++)
        {
            if (m_FlatRows[i].index == index)
            {
                flatIdx = i;
                break;
            }
        }

        if (flatIdx == -1)
            return;

        if (m_ExpandedItems.Contains(index))
        {
            // --- Collapse: remove all descendant rows in a single O(n) pass ---
            m_ExpandedItems.Remove(index);

            int nodeDepth    = m_FlatRows[flatIdx].depth;
            int removeStart  = flatIdx + 1;
            int removeEnd    = removeStart;

            while (removeEnd < (int)m_FlatRows.Size() && m_FlatRows[removeEnd].depth > nodeDepth)
                removeEnd++;

            int i = 0;
            m_FlatRows.RemoveAll([removeStart, removeEnd, &i](const FTreeViewFlatRow&)
            {
                int cur = i++;
                return cur >= removeStart && cur < removeEnd;
            });
        }
        else
        {
            // --- Expand: collect child rows, then batch-insert in O(n + k) ---
            m_ExpandedItems.Add(index);

            TArray<FTreeViewFlatRow> toInsert;
            CollectRows(index, m_FlatRows[flatIdx].depth + 1, toInsert);

            int insertPos = flatIdx + 1;
            int k         = (int)toInsert.Size();
            int n         = (int)m_FlatRows.Size();

            // Resize to make room, shift tail right by k, then fill
            m_FlatRows.Resize(n + k);
            for (int j = n - 1; j >= insertPos; j--)
                m_FlatRows[j + k] = std::move(m_FlatRows[j]);
            for (int j = 0; j < k; j++)
                m_FlatRows[insertPos + j] = std::move(toInsert[j]);
        }

        MarkLayoutDirty();
    }

    void FTreeViewContent::CollectRows(FModelIndex parent, int depth, TArray<FTreeViewFlatRow>& out)
    {
        Ref<FTreeView> treeView = GetTreeView();
        if (!treeView)
            return;

        Ref<FItemModel> model = treeView->Model();
        if (!model)
            return;

        u32 count = model->GetRowCount(parent);
        for (u32 i = 0; i < count; i++)
        {
            FModelIndex index  = model->GetIndex(i, 0, parent);
            u32 childCount     = model->GetRowCount(index);
            out.Add({ index, parent, depth, childCount > 0 });

            if (childCount > 0 && m_ExpandedItems.Contains(index))
                CollectRows(index, depth + 1, out);
        }
    }

    Ref<FScrollBox> FTreeViewContent::GetParentScrollBox()
    {
        if (Ref<FWidget> parent = GetParentWidget())
        {
            if (Ref<FScrollBox> scrollBox = parent->Cast<FScrollBox>())
            {
                return scrollBox;
            }
        }

        return nullptr;
    }

    void FTreeViewContent::UpdateVisibleRows(FVec2 finalSize)
    {
        Ref<FTreeView> treeView = GetTreeView();
        if (!treeView)
            return;

        Ref<FItemModel> model = treeView->Model();
        if (!model)
            return;

        Ref<FScrollBox> scrollBox = GetParentScrollBox();
        if (!scrollBox)
            return;

        const int columnCount = model->GetColumnCount();

        f32  rowHeight    = treeView->RowHeight();
        f32  scrollY      = scrollBox->ScrollOffset().y;
        f32  viewportH    = scrollBox->GetViewportSize().y;

        int firstRow = (int)(scrollY / rowHeight);
        int lastRow  = FMath::Min((int)((scrollY + viewportH) / rowHeight) + 1,
                                  (int)m_FlatRows.Size() - 1);

        m_FirstVisibleRow = firstRow;  // cache for ToggleExpanded O(1) lookup

        int needed = lastRow - firstRow + 1;

        // Grow pool dynamically
        while ((int)m_Rows.Size() < needed)
        {
            Ref<FTreeViewRow> row;
            FAssignNew(FTreeViewRow, row)
            .SubStyle("Row");
            row->m_TreeView = treeView;
            AddChildWidget(row);
            m_Rows.Add(row);
        }

        // Hide pool rows not needed this frame
        for (int i = needed; i < (int)m_Rows.Size(); i++)
            m_Rows[i]->Excluded(true);

        TArray<FModelIndex> columns;
        columns.Resize(columnCount);

        for (int i = firstRow; i <= lastRow; i++)
        {
            Ref<FTreeViewRow> row = m_Rows[i - firstRow];
            row->Excluded(false);
            row->m_FlatRowIndex = i;
            FModelIndex index = m_FlatRows[i].index;
            row->m_RowIndex = index;
            row->SubStyle(i % 2 == 0 ? "RowAlternate" : "Row");
            if (index.IsValid())
            {
                const FModelIndex& parentIndex = m_FlatRows[i].parentIndex;

                for (int j = 0; j < columnCount; j++)
                    columns[j] = model->GetIndex(index.Row(), j, parentIndex);

                row->SetData(columns);
            }
            else
            {
                row->SetData({});
            }

            f32 y = i * rowHeight;
            f32 x = m_FlatRows[i].depth * treeView->RowIndentWidth();
            row->Padding(FMargin(x, 0, 0, 0));
            row->SetLayoutPosition(FVec2(0, y));
            row->ArrangeContent(FVec2(finalSize.x, rowHeight));  // full width, padding handles indent
        }
    }

} // namespace Fusion

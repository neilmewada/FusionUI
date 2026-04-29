#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FTreeViewRow;
    class FTreeView;

    class FUSIONWIDGETS_API FTreeViewContent : public FContainerWidget
    {
        FUSION_WIDGET(FTreeViewContent, FContainerWidget)
    protected:

        FTreeViewContent();

        void Construct() override;

    public:

        Ref<FTreeView> GetTreeView() const { return m_TreeView.Lock(); }

        void OnModelChanged(Ref<FItemModel> model);

        void OnScrollOffsetChanged(FVec2 offset);

        bool IsExpanded(FModelIndex index) const
        {
            return m_ExpandedItems.Contains(index);
        }

        // - Layout -

        FVec2 MeasureContent(FVec2 availableSize) override;

        void ArrangeContent(FVec2 finalSize) override;

    protected:

        struct FTreeViewFlatRow
        {
            FModelIndex index;
            FModelIndex parentIndex;    // cached to avoid GetParent() calls during scroll
            int         depth;
            bool        hasChildren;
            u64         continuationMask = 0; // bit d = 1 → ancestor at depth d has more siblings after this row
        };

        Ref<FScrollBox> GetParentScrollBox();

        void RebuildFlatRows();
        void AppendRows(FModelIndex parent, int depth, u64 parentMask);
        void CollectRows(FModelIndex parent, int depth, TArray<FTreeViewFlatRow>& out, u64 parentMask);

        void UpdateVisibleRows(FVec2 finalSize);

    public:

        // Takes the flat row index directly — O(1), no linear search.
        void ToggleExpanded(int flatIdx);

    protected:

        WeakRef<FTreeView> m_TreeView;

        TArray<FTreeViewFlatRow> m_FlatRows;
        THashSet<FModelIndex> m_ExpandedItems;
        TArray<Ref<FTreeViewRow>> m_Rows;
        FVec2 m_CachedFinalSize;
        int   m_FirstVisibleRow = 0;

    public:

        FUSION_PROPERTY_SET(Ref<FTreeView>, TreeView)
        {
            self.m_TreeView = value;
            return self;
        }

        friend class FTreeViewRow;
    };
    
} // namespace Fusion

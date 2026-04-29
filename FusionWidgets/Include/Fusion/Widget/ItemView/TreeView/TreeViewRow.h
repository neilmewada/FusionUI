#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    class FUSIONWIDGETS_API FTreeViewRow : public FDecoratedBox
    {
        FUSION_WIDGET(FTreeViewRow, FDecoratedBox)
    protected:

        FTreeViewRow();

        void Construct() override;

    public:

        Ref<FTreeView> GetTreeView() const { return m_TreeView.Lock(); }

        int GetFlatRowIndex() const { return m_FlatRowIndex; }

        void Paint(FPainter& painter) override;

        void SetData(const TArray<FModelIndex>& columns);

        void OnMouseEnter(FMouseEvent& event) override;
        FEventReply OnMouseMove(FMouseEvent& event) override;
        void OnMouseLeave(FMouseEvent& event) override;
        FEventReply OnMouseButtonDown(FMouseEvent& event) override;

    protected:

        Ref<FHorizontalStack> m_HStack;
        WeakRef<FTreeView> m_TreeView;
        TArray<FModelIndex> m_Columns;
        int m_FlatRowIndex = 0;
        FModelIndex m_RowIndex;

    private:

        friend class FTreeView;
        friend class FTreeViewContent;
    };
    
} // namespace Fusion

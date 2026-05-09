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

        bool IsSelected() const { return TestStyleState(EStyleState::Selected); }

        void Paint(FPainter& painter) override;

        void SetData(const TArray<FModelIndex>& columns);

        void ResetState();

        void OnMouseEnter(FMouseEvent& event) override;
        FEventReply OnMouseMove(FMouseEvent& event) override;
        void OnMouseLeave(FMouseEvent& event) override;
        FEventReply OnMouseButtonDown(FMouseEvent& event) override;
        FEventReply OnMouseButtonUp(FMouseEvent& event) override;

    protected:

        enum class EHoveredCellItem
        {
            None = 0,
            Chevron,
            Icon
        };

        Ref<FHorizontalStack>   m_HStack;
        WeakRef<FTreeView>      m_TreeView;
        TArray<FModelIndex>     m_Cells;
        TArray<FItemViewLayout> m_CellLayouts;

        bool             m_IsMousePressed   = false;
        int              m_HoveredCellIndex = -1;
        EHoveredCellItem m_HoveredCellItem  = EHoveredCellItem::None;

        int m_FlatRowIndex      = 0;
        int m_Depth             = 0;
        u64 m_ContinuationMask  = 0;
        FModelIndex m_RowIndex  = {};

    public:

        // - Fusion Properties -

        FUSION_STYLE_PROPERTIES(
            (FColor, ChevronColor,          Paint),
            (FColor, ChevronHoverColor,     Paint),
            (FColor, ChevronPressedColor,   Paint)
        );

        friend class FTreeView;
        friend class FTreeViewContent;
    };
    
} // namespace Fusion

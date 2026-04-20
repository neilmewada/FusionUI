#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FScrollBox;

    FUSION_SIGNAL_TYPE(FScrollBoxSignal, FScrollBox* sender);

    enum class EScrollbarVisibility : u8
    {
        Auto = 0, AlwaysVisible, AlwaysHidden
    };

    class FUSIONWIDGETS_API FScrollBox : public FDecoratedBox
    {
        FUSION_WIDGET(FScrollBox, FDecoratedBox)
    protected:

        FScrollBox();

    public:

        FVec2 GetMaxScroll() const { return m_MaxScroll; }

        // - Layout -

        bool IsLayoutBoundary() override;

        FVec2 MeasureContent(FVec2 availableSize) override;

        void ArrangeContent(FVec2 finalSize) override;

        // - Paint -

        void PaintOverContent(FPainter& painter) override;

        // - Hit Testing -

        bool ShouldHitTestChildren(FVec2 localMousePos) override;

        // - Events -

        FEventReply OnMouseButtonDown(FMouseEvent& event) override;
        FEventReply OnMouseButtonUp(FMouseEvent& event) override;
        FEventReply OnMouseMove(FMouseEvent& event) override;
        void OnMouseLeave(FMouseEvent& event) override;
        FEventReply OnMouseWheel(FMouseEvent& event) override;

    protected:

        FUSION_STYLE_PROPERTIES(
            (FBrush, TrackBackground,        Paint),
            (FShape, TrackShape,             Paint),
            (FBrush, ThumbBackground,        Paint),
            (FBrush, ThumbHoverBackground,   Paint),
            (FBrush, ThumbPressedBackground, Paint),
            (FShape, ThumbShape,             Paint),
            (f32,     ScrollbarThickness,     Layout),
            (f32,     ScrollbarPadding,       Layout),
            (FMargin, ContentPadding,         Layout)
        );

        FUSION_LAYOUT_PROPERTY(FVec2,               ScrollOffset);
        FUSION_LAYOUT_PROPERTY(bool,                CanScrollVertical);
        FUSION_LAYOUT_PROPERTY(bool,                CanScrollHorizontal);
        FUSION_LAYOUT_PROPERTY(f32,                 ScrollSpeed);
        FUSION_LAYOUT_PROPERTY(EScrollbarVisibility, HorizontalScrollVisibility);
        FUSION_LAYOUT_PROPERTY(EScrollbarVisibility, VerticalScrollVisibility);

        FUSION_SIGNAL(FSignal<void(FVec2)>, OnScrollOffsetChanged);

    private:

        // Computed during layout
        FVec2 m_ContentSize  = FVec2(0, 0);
        FVec2 m_ViewportSize = FVec2(0, 0);
        FVec2 m_MaxScroll    = FVec2(0, 0);

        // Scrollbar visibility (resolved during ArrangeContent)
        bool m_bShowVertScrollbar = false;
        bool m_bShowHorzScrollbar = false;

        // Cached scrollbar rects in local widget space (resolved during ArrangeContent)
        FRect m_VertTrackRect;
        FRect m_VertThumbRect;
        FRect m_HorzTrackRect;
        FRect m_HorzThumbRect;

        // Per-bar interaction state
        bool m_bVertThumbHovered = false;
        bool m_bVertThumbPressed = false;
        bool m_bHorzThumbHovered = false;
        bool m_bHorzThumbPressed = false;

        // Drag state
        bool  m_bDraggingVert     = false;
        bool  m_bDraggingHorz     = false;
        FVec2 m_DragStartOffset   = FVec2(0, 0);
        FVec2 m_DragStartMousePos = FVec2(0, 0);
    };

} // namespace Fusion

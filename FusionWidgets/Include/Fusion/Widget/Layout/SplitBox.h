#pragma once

namespace Fusion
{
    
    class FUSIONWIDGETS_API FSplitBox : public FStackBox
    {
        FUSION_WIDGET(FSplitBox, FStackBox)
    protected:

        FSplitBox();

    public:

        // - Cursor -

        FCursor GetActiveCursorAt(FVec2 localPos) override;

        // - Layout -

        void ArrangeContent(FVec2 finalSize) override;

        // - Events -

        bool ShouldHitTestChildren(FVec2 localMousePose) override;

        FEventReply OnMouseButtonDown(FMouseEvent& event) override;
        FEventReply OnMouseButtonUp(FMouseEvent& event) override;

        void OnMouseEnter(FMouseEvent& event) override;
        FEventReply OnMouseMove(FMouseEvent& event) override;
        void OnMouseLeave(FMouseEvent& event) override;

        // - Paint -

        void Paint(FPainter& painter) override;

    protected:

        FArray<FRect, 4> m_HandleRects;

        int  m_DraggedRect = -1;
        bool m_bIsDragHovered = false;
        bool m_bIsBeingResized = false;

        FUSION_STYLE_PROPERTIES(
            (FColor, SplitterHoverColor, Paint),
            (f32,    SplitterSizeRatio,  Paint)
        );
    };

} // namespace Fusion

#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    FScrollBox::FScrollBox() : Super()
    {
        m_CanScrollVertical          = true;
        m_CanScrollHorizontal        = false;
        m_ScrollOffset               = FVec2(0, 0);
        m_ScrollbarThickness         = 8.0f;
        m_ScrollbarPadding           = 2.0f;
        m_ScrollSpeed                = 40.0f;
        m_VerticalScrollVisibility   = EScrollbarVisibility::Auto;
        m_HorizontalScrollVisibility = EScrollbarVisibility::Auto;

        m_ClipContent = true;
    }

    // -------------------------------------------------
    // Layout
    // -------------------------------------------------

    FVec2 FScrollBox::MeasureContent(FVec2 availableSize)
    {
        // Reserve space for scrollbars on axes that may show them.
        // For Auto visibility we conservatively reserve the strip so the viewport
        // size is stable between measure and arrange.
        f32 vertStripW = (m_CanScrollVertical   && m_VerticalScrollVisibility   != EScrollbarVisibility::AlwaysHidden) ? m_ScrollbarThickness : 0.0f;
        f32 horzStripH = (m_CanScrollHorizontal && m_HorizontalScrollVisibility != EScrollbarVisibility::AlwaysHidden) ? m_ScrollbarThickness : 0.0f;

        if (GetChild() && !GetChild()->IsExcluded())
        {
            FMargin childMargin = GetChild()->Margin();

            // Give unlimited space on scrollable axes so the child reports its natural size.
            // Account for padding, content padding, and scrollbar strip on non-scrollable axes.
            FVec2 childAvailable = FVec2(
                m_CanScrollHorizontal ? 1e9f : FMath::Max(0.0f, availableSize.x - m_Padding.left - m_Padding.right - m_ContentPadding.left - m_ContentPadding.right - vertStripW - childMargin.left - childMargin.right),
                m_CanScrollVertical   ? 1e9f : FMath::Max(0.0f, availableSize.y - m_Padding.top  - m_Padding.bottom - m_ContentPadding.top  - m_ContentPadding.bottom - horzStripH - childMargin.top  - childMargin.bottom)
            );

            FVec2 childDesired = GetChild()->MeasureContent(childAvailable);

            m_ContentSize = FVec2(
                childDesired.x + childMargin.left + childMargin.right,
                childDesired.y + childMargin.top  + childMargin.bottom
            );
        }
        else
        {
            m_ContentSize = FVec2(0, 0);
        }

        // FScrollBox itself fills whatever space it is given.
        return m_DesiredSize = ApplyLayoutConstraints(availableSize);
    }

    void FScrollBox::ArrangeContent(FVec2 finalSize)
    {
        // Set m_LayoutSize via FWidget — skip FCompoundWidget's child arrangement.
        FWidget::ArrangeContent(finalSize);

        FVec2 layoutSize = GetLayoutSize();

        // The content area is the layout rect inset by Padding.
        // Scrollbars live inside the content area (on its right/bottom edge).
        // ContentPadding further insets the child within the viewport.
        const FVec2 contentAreaOrigin = FVec2(m_Padding.left, m_Padding.top);
        const FVec2 contentAreaSize   = FVec2(
            FMath::Max(0.0f, layoutSize.x - m_Padding.left - m_Padding.right),
            FMath::Max(0.0f, layoutSize.y - m_Padding.top  - m_Padding.bottom)
        );

        const FVec2 contentPaddingSize = FVec2(
            m_ContentPadding.left + m_ContentPadding.right,
            m_ContentPadding.top  + m_ContentPadding.bottom
        );

        // Determine scrollbar visibility (two-pass for Auto mode)
        // Scrollbars are based on the content area size (not affected by ContentPadding).
        FVec2 viewportSize = contentAreaSize;

        auto ShouldShowVert = [&]() -> bool
        {
            if (!m_CanScrollVertical) return false;
            if (m_VerticalScrollVisibility == EScrollbarVisibility::AlwaysVisible) return true;
            if (m_VerticalScrollVisibility == EScrollbarVisibility::AlwaysHidden)  return false;
            return m_ContentSize.y > viewportSize.y;
        };

        auto ShouldShowHorz = [&]() -> bool
        {
            if (!m_CanScrollHorizontal) return false;
            if (m_HorizontalScrollVisibility == EScrollbarVisibility::AlwaysVisible) return true;
            if (m_HorizontalScrollVisibility == EScrollbarVisibility::AlwaysHidden)  return false;
            return m_ContentSize.x > viewportSize.x;
        };

        // Pass 1
        m_bShowVertScrollbar = ShouldShowVert();
        m_bShowHorzScrollbar = ShouldShowHorz();

        if (m_bShowVertScrollbar) viewportSize.x -= m_ScrollbarThickness;
        if (m_bShowHorzScrollbar) viewportSize.y -= m_ScrollbarThickness;

        // Pass 2: shrinking the viewport on one axis may cause overflow on the other
        if (!m_bShowVertScrollbar && ShouldShowVert())
        {
            m_bShowVertScrollbar = true;
            viewportSize.x -= m_ScrollbarThickness;
        }
        if (!m_bShowHorzScrollbar && ShouldShowHorz())
        {
            m_bShowHorzScrollbar = true;
            viewportSize.y -= m_ScrollbarThickness;
        }

        m_ViewportSize = FVec2(FMath::Max(0.0f, viewportSize.x), FMath::Max(0.0f, viewportSize.y));

        // Clamp scroll offset.
        // The effective viewport for scroll range = viewport minus ContentPadding.
        const FVec2 effectiveViewport = FVec2(
            FMath::Max(0.0f, m_ViewportSize.x - contentPaddingSize.x),
            FMath::Max(0.0f, m_ViewportSize.y - contentPaddingSize.y)
        );

        FVec2 maxScroll = FVec2(
            FMath::Max(0.0f, m_ContentSize.x - effectiveViewport.x),
            FMath::Max(0.0f, m_ContentSize.y - effectiveViewport.y)
        );

        m_ScrollOffset = FVec2(
            m_CanScrollHorizontal ? FMath::Clamp(m_ScrollOffset.x, 0.0f, maxScroll.x) : 0.0f,
            m_CanScrollVertical   ? FMath::Clamp(m_ScrollOffset.y, 0.0f, maxScroll.y) : 0.0f
        );

        // Arrange content child
        if (GetChild() && !GetChild()->IsExcluded())
        {
            FMargin childMargin = GetChild()->Margin();

            FVec2 childPos = FVec2(
                contentAreaOrigin.x + m_ContentPadding.left - m_ScrollOffset.x + childMargin.left,
                contentAreaOrigin.y + m_ContentPadding.top  - m_ScrollOffset.y + childMargin.top
            );
            FVec2 childSize = FVec2(
                FMath::Max(effectiveViewport.x, m_ContentSize.x) - childMargin.left - childMargin.right,
                FMath::Max(effectiveViewport.y, m_ContentSize.y) - childMargin.top  - childMargin.bottom
            );

            GetChild()->SetLayoutPosition(childPos);
            GetChild()->ArrangeContent(childSize);
        }

        // -------------------------------------------------------
        // Compute scrollbar rects (relative to content area origin)
        // -------------------------------------------------------
        const f32 pad       = m_ScrollbarPadding;
        const f32 thickness = m_ScrollbarThickness;

        if (m_bShowVertScrollbar)
        {
            f32 trackX = contentAreaOrigin.x + m_ViewportSize.x + pad;
            f32 trackY = contentAreaOrigin.y + pad;
            f32 trackW = thickness - pad * 2.0f;
            f32 trackH = m_ViewportSize.y - pad * 2.0f;

            if (m_bShowHorzScrollbar)
                trackH -= thickness;

            m_VertTrackRect = FRect(trackX, trackY, trackX + trackW, trackY + trackH);

            f32 thumbRatio = (m_ContentSize.y > 0.0f) ? FMath::Clamp(m_ViewportSize.y / m_ContentSize.y, 0.0f, 1.0f) : 1.0f;
            f32 thumbH     = FMath::Max(thickness, trackH * thumbRatio);
            f32 thumbT     = (maxScroll.y > 0.0f) ? FMath::Clamp(m_ScrollOffset.y / maxScroll.y, 0.0f, 1.0f) : 0.0f;
            f32 thumbY     = trackY + thumbT * (trackH - thumbH);

            m_VertThumbRect = FRect(trackX, thumbY, trackX + trackW, thumbY + thumbH);
        }
        else
        {
            m_VertTrackRect = FRect();
            m_VertThumbRect = FRect();
        }

        if (m_bShowHorzScrollbar)
        {
            f32 trackX = contentAreaOrigin.x + pad;
            f32 trackY = contentAreaOrigin.y + m_ViewportSize.y + pad;
            f32 trackW = m_ViewportSize.x - pad * 2.0f;
            f32 trackH = thickness - pad * 2.0f;

            if (m_bShowVertScrollbar)
                trackW -= thickness;

            m_HorzTrackRect = FRect(trackX, trackY, trackX + trackW, trackY + trackH);

            f32 thumbRatio = (m_ContentSize.x > 0.0f) ? FMath::Clamp(m_ViewportSize.x / m_ContentSize.x, 0.0f, 1.0f) : 1.0f;
            f32 thumbW     = FMath::Max(thickness, trackW * thumbRatio);
            f32 thumbT     = (maxScroll.x > 0.0f) ? FMath::Clamp(m_ScrollOffset.x / maxScroll.x, 0.0f, 1.0f) : 0.0f;
            f32 thumbX     = trackX + thumbT * (trackW - thumbW);

            m_HorzThumbRect = FRect(thumbX, trackY, thumbX + thumbW, trackY + trackH);
        }
        else
        {
            m_HorzTrackRect = FRect();
            m_HorzThumbRect = FRect();
        }
    }

    // -------------------------------------------------
    // Paint
    // -------------------------------------------------

    void FScrollBox::PaintOverContent(FPainter& painter)
    {
        Super::PaintOverContent(painter); // handles outline from FDecoratedBox

        if (m_bShowVertScrollbar)
        {
            painter.SetPen(FPen());
            painter.SetBrush(TrackBackground());
            painter.FillShape(m_VertTrackRect, TrackShape());

            FBrush thumbBrush = m_bVertThumbPressed ? ThumbPressedBackground()
                              : m_bVertThumbHovered  ? ThumbHoverBackground()
                              : ThumbBackground();
            painter.SetBrush(thumbBrush);
            painter.FillShape(m_VertThumbRect, ThumbShape());
        }

        if (m_bShowHorzScrollbar)
        {
            painter.SetPen(FPen());
            painter.SetBrush(TrackBackground());
            painter.FillShape(m_HorzTrackRect, TrackShape());

            FBrush thumbBrush = m_bHorzThumbPressed ? ThumbPressedBackground()
                              : m_bHorzThumbHovered  ? ThumbHoverBackground()
                              : ThumbBackground();
            painter.SetBrush(thumbBrush);
            painter.FillShape(m_HorzThumbRect, ThumbShape());
        }
    }

    // -------------------------------------------------
    // Hit Testing
    // -------------------------------------------------

    bool FScrollBox::ShouldHitTestChildren(FVec2 localMousePos)
    {
        // localMousePos is already in local widget space (converted by HitTestWidget)
        if (m_bShowVertScrollbar && m_VertTrackRect.Contains(localMousePos))
            return false;
        if (m_bShowHorzScrollbar && m_HorzTrackRect.Contains(localMousePos))
            return false;
        return true;
    }

    // -------------------------------------------------
    // Mouse Events
    // -------------------------------------------------

    FEventReply FScrollBox::OnMouseButtonDown(FMouseEvent& event)
    {
        if (!event.IsLeftButton())
            return FEventReply::Unhandled();

        // Convert surface space → local widget space
        FVec2 localPos = GetCachedLayerSpaceTransform().Inverse().TransformPoint(event.MousePosition);

        if (m_bShowVertScrollbar && m_VertTrackRect.Contains(localPos))
        {
            if (m_VertThumbRect.Contains(localPos))
            {
                m_bDraggingVert      = true;
                m_bVertThumbPressed  = true;
                m_DragStartOffset    = m_ScrollOffset;
                m_DragStartMousePos  = localPos;
            }
            else
            {
                // Click on track — jump to position centred on click
                f32 trackH  = m_VertTrackRect.GetHeight();
                f32 thumbH  = m_VertThumbRect.GetHeight();
                f32 t       = FMath::Clamp((localPos.y - m_VertTrackRect.top - thumbH * 0.5f) / FMath::Max(1.0f, trackH - thumbH), 0.0f, 1.0f);
                f32 maxScrollY = FMath::Max(0.0f, m_ContentSize.y - m_ViewportSize.y);

                m_ScrollOffset.y = t * maxScrollY;
                MarkLayoutDirty();
                m_OnScrollOffsetChanged.ExecuteIfBound(m_ScrollOffset);
            }

            MarkPaintDirty();
            if (m_bDraggingVert)
				return FEventReply::Handled().CaptureMouse();
            return FEventReply::Handled();
        }

        if (m_bShowHorzScrollbar && m_HorzTrackRect.Contains(localPos))
        {
            if (m_HorzThumbRect.Contains(localPos))
            {
                m_bDraggingHorz      = true;
                m_bHorzThumbPressed  = true;
                m_DragStartOffset    = m_ScrollOffset;
                m_DragStartMousePos  = localPos;
            }
            else
            {
                // Click on track — jump to position
                f32 trackW  = m_HorzTrackRect.GetWidth();
                f32 thumbW  = m_HorzThumbRect.GetWidth();
                f32 t       = FMath::Clamp((localPos.x - m_HorzTrackRect.left - thumbW * 0.5f) / FMath::Max(1.0f, trackW - thumbW), 0.0f, 1.0f);
                f32 maxScrollX = FMath::Max(0.0f, m_ContentSize.x - m_ViewportSize.x);

                m_ScrollOffset.x = t * maxScrollX;
                MarkLayoutDirty();
                m_OnScrollOffsetChanged.ExecuteIfBound(m_ScrollOffset);
            }

            MarkPaintDirty();
            if (m_bDraggingHorz)
                return FEventReply::Handled().CaptureMouse();
            return FEventReply::Handled();
        }

        return FEventReply::Unhandled();
    }

    FEventReply FScrollBox::OnMouseButtonUp(FMouseEvent& event)
    {
        if (!event.IsLeftButton())
            return FEventReply::Unhandled();

        if (m_bDraggingVert || m_bDraggingHorz)
        {
            m_bDraggingVert     = false;
            m_bDraggingHorz     = false;
            m_bVertThumbPressed = false;
            m_bHorzThumbPressed = false;
            MarkPaintDirty();
            return FEventReply::Handled().ReleaseMouse();
        }

        return FEventReply::Unhandled();
    }

    FEventReply FScrollBox::OnMouseMove(FMouseEvent& event)
    {
        FVec2 localPos = GetCachedLayerSpaceTransform().Inverse().TransformPoint(event.MousePosition);

        if (m_bDraggingVert)
        {
            f32 trackH     = m_VertTrackRect.GetHeight();
            f32 thumbH     = m_VertThumbRect.GetHeight();
            f32 maxScrollY = FMath::Max(0.0f, m_ContentSize.y - m_ViewportSize.y);
            f32 usable     = trackH - thumbH;

            if (usable > 0.0f)
            {
                f32 delta = localPos.y - m_DragStartMousePos.y;
                m_ScrollOffset.y = FMath::Clamp(m_DragStartOffset.y + (delta / usable) * maxScrollY, 0.0f, maxScrollY);
                MarkLayoutDirty();
                MarkPaintDirty();
                m_OnScrollOffsetChanged.ExecuteIfBound(m_ScrollOffset);
            }

            return FEventReply::Handled();
        }

        if (m_bDraggingHorz)
        {
            f32 trackW     = m_HorzTrackRect.GetWidth();
            f32 thumbW     = m_HorzThumbRect.GetWidth();
            f32 maxScrollX = FMath::Max(0.0f, m_ContentSize.x - m_ViewportSize.x);
            f32 usable     = trackW - thumbW;

            if (usable > 0.0f)
            {
                f32 delta = localPos.x - m_DragStartMousePos.x;
                m_ScrollOffset.x = FMath::Clamp(m_DragStartOffset.x + (delta / usable) * maxScrollX, 0.0f, maxScrollX);
                MarkLayoutDirty();
                MarkPaintDirty();
                m_OnScrollOffsetChanged.ExecuteIfBound(m_ScrollOffset);
            }

            return FEventReply::Handled();
        }

        // Update hover states
        bool vertHovered = m_bShowVertScrollbar && m_VertThumbRect.Contains(localPos);
        bool horzHovered = m_bShowHorzScrollbar && m_HorzThumbRect.Contains(localPos);

        if (vertHovered != m_bVertThumbHovered || horzHovered != m_bHorzThumbHovered)
        {
            m_bVertThumbHovered = vertHovered;
            m_bHorzThumbHovered = horzHovered;
            MarkPaintDirty();
        }

        return FEventReply::Unhandled();
    }

    void FScrollBox::OnMouseLeave(FMouseEvent& event)
    {
        // Keep drag state active — user may have moved outside widget bounds while dragging.
        // Only clear hover when not dragging.
        if (!m_bDraggingVert && !m_bDraggingHorz)
        {
            if (m_bVertThumbHovered || m_bHorzThumbHovered)
            {
                m_bVertThumbHovered = false;
                m_bHorzThumbHovered = false;
                MarkPaintDirty();
            }
        }
    }

    FEventReply FScrollBox::OnMouseWheel(FMouseEvent& event)
    {
        FVec2 maxScroll = FVec2(
            FMath::Max(0.0f, m_ContentSize.x - m_ViewportSize.x),
            FMath::Max(0.0f, m_ContentSize.y - m_ViewportSize.y)
        );

        bool scrolled = false;

        if (m_CanScrollVertical && event.WheelDelta.y != 0.0f)
        {
            f32 newY = FMath::Clamp(m_ScrollOffset.y - event.WheelDelta.y * m_ScrollSpeed, 0.0f, maxScroll.y);
            if (newY != m_ScrollOffset.y)
            {
                m_ScrollOffset.y = newY;
                scrolled = true;
            }
        }

        if (m_CanScrollHorizontal && event.WheelDelta.x != 0.0f)
        {
            f32 newX = FMath::Clamp(m_ScrollOffset.x - event.WheelDelta.x * m_ScrollSpeed, 0.0f, maxScroll.x);
            if (newX != m_ScrollOffset.x)
            {
                m_ScrollOffset.x = newX;
                scrolled = true;
            }
        }

        if (scrolled)
        {
            MarkLayoutDirty();
            MarkPaintDirty();
            m_OnScrollOffsetChanged.ExecuteIfBound(m_ScrollOffset);
            return FEventReply::Handled();
        }

        return FEventReply::Unhandled();
    }

} // namespace Fusion

#include "Fusion/Widgets.h"

namespace Fusion
{
	FSplitBox::FSplitBox()
	{
		m_Spacing = 5.0f;
		m_SplitterSizeRatio = 1.5f;
		m_SplitterHoverColor = FColors::White.WithAlpha(0.5f);
	}

	FCursor FSplitBox::GetActiveCursorAt(FVec2 localPos)
	{
		if (GetChildCount() <= 1)
			return Super::GetActiveCursorAt(localPos);

		ESystemCursor resizeCursor = Direction() == EStackDirection::Horizontal
			? ESystemCursor::SizeHorizontal
			: ESystemCursor::SizeVertical;

		for (const FRect& handle : m_HandleRects)
		{
			if (handle.Contains(localPos))
				return FCursor::System(resizeCursor);
		}

		return Super::GetActiveCursorAt(localPos);
	}

	void FSplitBox::ArrangeContent(FVec2 finalSize)
	{
		Super::ArrangeContent(finalSize);

		m_HandleRects.Clear();

		if (GetChildCount() <= 1)
			return;

		for (u32 i = 0; i < GetChildCount() - 1; i++)
		{
			Ref<FWidget> cur = GetChildAt(i);
			Ref<FWidget> next = GetChildAt(i + 1);

			if (!cur || !next || cur->IsExcluded() || next->IsExcluded())
				continue;

			if (Direction() == EStackDirection::Horizontal)
			{
				f32 gapLeft = cur->GetLayoutPosition().x + cur->GetLayoutSize().x;
				f32 gapRight = next->GetLayoutPosition().x;
				f32 dist = gapRight - gapLeft;
				f32 diff = (dist * SplitterSizeRatio()) - dist;

				m_HandleRects.Add(FRect(gapLeft - diff / 2, 0.0f, gapRight + diff, GetLayoutSize().y));
			}
			else
			{
				f32 gapTop = cur->GetLayoutPosition().y + cur->GetLayoutSize().y;
				f32 gapBottom = next->GetLayoutPosition().y;
				f32 dist = gapBottom - gapTop;
				f32 diff = (dist * SplitterSizeRatio()) - dist;

				m_HandleRects.Add(FRect(0.0f, gapTop - diff / 2, GetLayoutSize().x, gapBottom + diff / 2));
			}
		}
	}

	bool FSplitBox::ShouldHitTestChildren(FVec2 localMousePos)
	{
		for (const FRect& handle : m_HandleRects)
		{
			if (handle.Contains(localMousePos))
				return false;
		}

		return Super::ShouldHitTestChildren(localMousePos);
	}

	FEventReply FSplitBox::OnMouseButtonDown(FMouseEvent& event)
	{
		if (!event.IsLeftButton())
			return Super::OnMouseButtonDown(event);

		FVec2 localPos = GetGlobalTransform().Inverse().TransformPoint(event.MousePosition);

		ESystemCursor resizeCursor = Direction() == EStackDirection::Horizontal
			? ESystemCursor::SizeHorizontal
			: ESystemCursor::SizeVertical;

		for (const FRect& handle : m_HandleRects)
		{
			if (handle.Contains(localPos))
			{
				m_bIsBeingResized = true;
				return FEventReply::Handled().CaptureMouse(FCursor::System(resizeCursor));
			}
		}

		return Super::OnMouseButtonDown(event);
	}

	FEventReply FSplitBox::OnMouseButtonUp(FMouseEvent& event)
	{
		if (event.IsLeftButton() && m_bIsBeingResized)
		{
			m_bIsBeingResized = false;
			return FEventReply::Handled().ReleaseMouse();
		}
		return Super::OnMouseButtonUp(event);
	}

	void FSplitBox::OnMouseEnter(FMouseEvent& event)
	{
		Super::OnMouseEnter(event);

		FVec2 localPos = GetGlobalTransform().Inverse().TransformPoint(event.MousePosition);

		bool hovered = false;
		int draggedRect = -1;

		for (const FRect& handle : m_HandleRects)
		{
			draggedRect++;

			if (handle.Contains(localPos))
			{
				hovered = true;
				break;
			}
		}

		if (m_bIsDragHovered != hovered || (hovered && m_DraggedRect != draggedRect))
		{
			m_bIsDragHovered = hovered;

			m_DraggedRect = hovered ? draggedRect : -1;

			MarkPaintDirty();
		}
	}

	FEventReply FSplitBox::OnMouseMove(FMouseEvent& event)
	{
		FVec2 localPos = GetGlobalTransform().Inverse().TransformPoint(event.MousePosition);

		bool hovered = false;
		int draggedRect = -1;

		for (const FRect& handle : m_HandleRects)
		{
			draggedRect++;

			if (handle.Contains(localPos))
			{
				hovered = true;
				break;
			}
		}

		if (m_bIsBeingResized)
		{
			
		}
		else if (m_bIsDragHovered != hovered && !m_bIsBeingResized)
		{
			m_bIsDragHovered = hovered;

			m_DraggedRect = hovered ? draggedRect : -1;

			MarkPaintDirty();

			return FEventReply::Handled();
		}

		return Super::OnMouseMove(event);
	}

	void FSplitBox::OnMouseLeave(FMouseEvent& event)
	{
		Super::OnMouseLeave(event);

		if (m_bIsDragHovered && !m_bIsBeingResized)
		{
			m_bIsDragHovered = false;
			m_DraggedRect = -1;

			MarkPaintDirty();
		}
	}

	void FSplitBox::Paint(FPainter& painter)
	{
		Super::Paint(painter);

		if (m_bIsDragHovered && m_DraggedRect >= 0 && m_DraggedRect < (int)m_HandleRects.Size())
		{
			painter.SetBrush(SplitterHoverColor());
			painter.SetPen(FPen());

			FVec2 expansion = Direction() == EStackDirection::Horizontal 
				? FVec2(-m_HandleRects[m_DraggedRect].GetWidth() * SplitterSizeRatio() / 2, 0)
				: FVec2(0, -m_HandleRects[m_DraggedRect].GetHeight() * SplitterSizeRatio() / 2);

			painter.FillShape(m_HandleRects[m_DraggedRect].Expand(expansion), FRectangle());
		}
	}

} // namespace Fusion

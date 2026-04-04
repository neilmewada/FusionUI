#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	FLayer::FLayer(FName name, FObject* outer) : Super(name)
	{
		
	}

	f32 FLayer::GetDpiScale()
	{
		if (Ref<FWidget> widget = m_OwningWidget.Lock())
		{
			if (Ref<FSurface> surface = widget->GetParentSurface())
			{
				return surface->GetDpiScale();
			}
		}
		
		return 1.0f;
	}

	bool FLayer::NeedsRepaint()
	{
		if (Ref<FWidget> widget = m_OwningWidget.Lock())
		{
			return widget->IsPaintDirty();
		}
		return false;
	}

	void FLayer::DoPaintIfNeeded()
	{
		if (NeedsRepaint())
		{
			DoPaint();
		}
		else
		{
			for (auto layer : m_Children)
			{
				layer->DoPaintIfNeeded();
			}
		}
	}

	FAffineTransform FLayer::GetGlobalTransform()
	{
		FAffineTransform global = GetTransformInParentSpace();

		Ref<FLayer> parent = m_Parent.Lock();

		while (parent != nullptr)
		{
			global = parent->GetTransformInParentSpace() * global;

			parent = parent->m_Parent.Lock();
		}

		return global;
	}

	void FLayer::DoPaint()
	{
		if (Ref<FWidget> widget = GetOwningWidget())
		{
			FPainter painter{ this };

			m_DrawList.Clear();
			m_SplitPoints.Clear();

			// Default draw item always exists
			m_DrawList.drawItemArray[0].clipRectIndex = -1;

			DoPaint(widget, painter);

			m_DrawList.Finalize();
		}
	}

	void FLayer::DoPaint(Ref<FWidget> widget, FPainter& painter)
	{
		if (widget == nullptr || widget->IsFaulted())
			return;

		// If a widget in the children hierarchy is a paint boundary
		if (widget != GetOwningWidget().Get() && widget->IsPaintBoundary())
		{
			if (Ref<FLayerTree> tree = m_OwnerTree.Lock())
			{
				if (Ref<FLayer> layer = tree->FindLayerForWidget(widget->GetUuid()))
				{
					m_SplitPoints.Add(m_DrawList.GetCurrentDrawCmdCount());
					m_DrawList.NewDrawCmd();

					layer->m_CachedTransformInParentLayerSpace = painter.GetCurrentTransform();

					layer->m_NeedsCompositing = true;
					layer->DoPaintIfNeeded();
				}
			}

			return;
		}

		m_NeedsCompositing = true;

		FVec2 pivot = widget->Pivot() * widget->GetLayoutSize();

		painter.PushTransform(
			FAffineTransform::Translation(widget->GetLayoutPosition()) *
			FAffineTransform::Translation(pivot) *
			widget->Transform() *
			FAffineTransform::Translation(-pivot)
		);

		widget->m_CachedLayerSpaceTransform = painter.GetCurrentTransform();

		// Own AABB: transform the [0,0 → layoutSize] rect into layer space
		widget->m_CachedLayerSpaceAABB = widget->m_CachedLayerSpaceTransform.TransformAABB(
			FRect(FVec2(0, 0), widget->GetLayoutSize())
		);

		painter.ResetState();

		widget->SetWidgetFlag(EWidgetFlags::PaintDirty, false);
		widget->Paint(painter);

		bool clipPushed = false;

		if (widget->ClipContent())
		{
			painter.PushClip(FRect(FVec2(), widget->GetLayoutSize()), widget->ClipShape());
			clipPushed = true;
		}

		for (u32 i = 0; i < widget->GetChildCount(); i++)
		{
			if (Ref<FWidget> child = widget->GetChildAt(i))
			{
				DoPaint(child.Get(), painter);

				// We only include children in AABB if clipping is disabled
				if (widget->ClipContent())
					continue;

				// painter.GetCurrentTransform() is always the parent widget's transform here,
				// whether child was paint boundary (no push/pop) or normal (pushed then popped).
				if (child->IsPaintBoundary())
				{
					// child->cachedLayerSpaceAABB is in child layer space; convert to this layer
					widget->m_CachedLayerSpaceAABB = FRect::Union(
						widget->m_CachedLayerSpaceAABB,
						painter.GetCurrentTransform().TransformAABB(child->m_CachedLayerSpaceAABB)
					);
				}
				else
				{
					widget->m_CachedLayerSpaceAABB = FRect::Union(
						widget->m_CachedLayerSpaceAABB,
						child->m_CachedLayerSpaceAABB
					);
				}
			}
		}

		if (clipPushed)
		{
			painter.PopClip();
		}

		painter.SetBrush(FBrush());
		painter.SetPen(FPen());

		widget->PaintOverlay(painter);

		painter.PopTransform();
	}
} // namespace Fusion

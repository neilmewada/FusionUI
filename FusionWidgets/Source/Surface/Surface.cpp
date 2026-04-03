#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	FSurface::FSurface(FObject* outer) : Super("Surface", outer)
	{
		m_LayerTree = NewObject<FLayerTree>();
	}

	void FSurface::Initialize()
	{
		Ref<FApplicationInstance> application = GetApplication();
		FUSION_ASSERT(application.IsValid(), "Application Instance not found!");

		m_RenderCapabilities = application->GetRenderCapabilities();
	}

	void FSurface::Shutdown()
	{
		Ref<FApplicationInstance> application = GetApplication();
		FUSION_ASSERT(application.IsValid(), "Application Instance not found!");

		if (m_RenderTarget.IsValid())
		{
			application->ReleaseRenderTarget(m_RenderTarget);
		}
	}

	void FSurface::TickSurface()
	{
		if (!m_RootWidget || m_RenderTarget.IsNull())
			return;

		Ref<FApplicationInstance> application = GetApplication();
		if (!application)
			return;

		ZoneScoped;

		// - Layout

		FHashSet<FWidget*> pendingSet;
		for (auto& root : m_PendingLayoutRoots)
			pendingSet.Add(root.Get());

		// Remove any root whose ancestor is also pending
		m_PendingLayoutRoots.RemoveAll([&](Ref<FWidget> root)
			{
				Ref<FWidget> ancestor = root->GetParentWidget();
				while (ancestor != nullptr)
				{
					if (pendingSet.Contains(ancestor.Get()))
					{
						m_PendingLayoutRootIds.Remove(root->GetUuid());
						return true;
					}
					ancestor = ancestor->GetParentWidget();
				}
				return false;
			});

		for (int i = m_PendingLayoutRoots.Size() - 1; i >= 0; i--)
		{
			Ref<FWidget> root = m_PendingLayoutRoots[i];
			m_PendingLayoutRoots.RemoveAt(i);
			if (!root)
				continue;

			m_PendingLayoutRootIds.Remove(root->GetUuid());

			if (root->IsFaulted())
				continue;

			FVec2 availableSize = GetAvailableSize();
			if (Ref<FWidget> parentWidget = root->GetParentWidget())
			{
				availableSize.x = FMath::Max(0.0f, parentWidget->GetLayoutSize().x - parentWidget->Padding().left - parentWidget->Padding().right);
				availableSize.y = FMath::Max(0.0f, parentWidget->GetLayoutSize().y - parentWidget->Padding().top - parentWidget->Padding().bottom);
			}

			root->MeasureContent(availableSize);
			root->ArrangeContent(availableSize);
		}

		// - Layer Tree Sync

		m_LayerTree->DoSyncIfNeeded(m_RootWidget.Get());

		// - Paint

		m_LayerTree->DoPaintIfNeeded();

		// - Composite

		IntrusivePtr<FRenderSnapshot> snapshot = new FRenderSnapshot();

		CompositeLayer(snapshot, m_LayerTree->GetRootLayer(), 0);

		snapshot->viewData.PixelResolution = m_PixelSize;
		snapshot->viewData.ProjectionMatrix = FMat4::OrthographicProjection(
			0, m_AvailableSize.x,
			0, m_AvailableSize.y,
			-1.0f, 1.0f
		);
		snapshot->viewData.ViewMatrix = FMat4::Identity();
		snapshot->viewData.ViewProjectionMatrix = snapshot->viewData.ProjectionMatrix * snapshot->viewData.ViewMatrix;

		application->SubmitSnapshot(m_RenderTarget, snapshot);
	}

	void FSurface::SetOwningWidget(Ref<FWidget> widget)
	{
		if (m_RootWidget == widget)
			return;

		if (m_RootWidget)
		{
			m_RootWidget->SetParentSurfaceRecursive(nullptr);
		}

		m_RootWidget = widget;

		if (m_RootWidget)
		{
			m_RootWidget->SetParentSurfaceRecursive(this);

			AddPendingLayoutRoot(m_RootWidget);

			m_RootWidget->UpdateBoundaryFlags();

			m_RootWidget->MarkLayoutDirty();
			m_RootWidget->MarkPaintDirty();
		}
	}

	void FSurface::AddPendingLayoutRoot(Ref<FWidget> layoutRoot)
	{
		if (!layoutRoot)
			return;

		if (m_PendingLayoutRootIds.Contains(layoutRoot->GetUuid()))
			return;

		m_PendingLayoutRoots.Add(layoutRoot);
		m_PendingLayoutRootIds.Add(layoutRoot->GetUuid());
	}

	void FSurface::MarkRootLayoutDirty()
	{
		if (!m_RootWidget)
			return;

		AddPendingLayoutRoot(m_RootWidget);

		m_RootWidget->MarkLayoutDirty();
		m_RootWidget->MarkPaintDirty();
	}

	void FSurface::MarkLayerTreeDirty()
	{
		m_LayerTree->MarkSyncNeeded();
	}

	void FSurface::CompositeLayer(IntrusivePtr<FRenderSnapshot> snapshot, Ref<FLayer> layer, int layerIndex)
	{
		ZoneScoped;

		layer->m_NeedsCompositing = false;

		const SizeT constantBufferAlignment = m_RenderCapabilities.MinConstantBufferOffsetAlignment;
		const SizeT structuredBufferAlignment = m_RenderCapabilities.MinStructuredBufferOffsetAlignment;

		FUIDrawList* drawList = layer->GetDrawList();

		snapshot->vertexSplits.Insert({ .StartOffset = snapshot->vertexArray.GetByteSize(), .ByteSize = drawList->vertexArray.GetByteSize() });
		snapshot->vertexArray.Insert(drawList->vertexArray.GetData(), (int)drawList->vertexArray.GetCount());

		snapshot->indexSplits.Insert({ .StartOffset = snapshot->indexArray.GetByteSize(), .ByteSize = drawList->indexArray.GetByteSize() });
		snapshot->indexArray.Insert(drawList->indexArray.GetData(), (int)drawList->indexArray.GetCount());

		{
			SizeT alignedOffset = FMemoryUtils::AlignUp(snapshot->drawItemArray.GetByteSize(), structuredBufferAlignment);
			snapshot->drawItemArray.InsertRange((int)((alignedOffset - snapshot->drawItemArray.GetByteSize()) / sizeof(FUIDrawItem)));
			snapshot->drawItemSplits.Insert({ .StartOffset = alignedOffset, .ByteSize = drawList->drawItemArray.GetByteSize() });
			snapshot->drawItemArray.Insert(drawList->drawItemArray.GetData(), (int)drawList->drawItemArray.GetCount());
		}

		snapshot->drawCmdSplits.Insert({ .StartOffset = snapshot->drawCmdArray.GetByteSize(), .ByteSize = drawList->drawCmdArray.GetByteSize() });
		snapshot->drawCmdArray.Insert(drawList->drawCmdArray.GetData(), (int)drawList->drawCmdArray.GetCount());

		{
			SizeT alignedOffset = FMemoryUtils::AlignUp(snapshot->clipRectArray.GetByteSize(), structuredBufferAlignment);
			snapshot->clipRectArray.InsertRange((int)((alignedOffset - snapshot->clipRectArray.GetByteSize()) / sizeof(FUIClipRect)));
			snapshot->clipRectSplits.Insert({ .StartOffset = alignedOffset, .ByteSize = drawList->clipRectArray.GetByteSize() });
			snapshot->clipRectArray.Insert(drawList->clipRectArray.GetData(), (int)drawList->clipRectArray.GetCount());
		}

		{
			SizeT alignedOffset = FMemoryUtils::AlignUp(snapshot->gradientStopArray.GetByteSize(), structuredBufferAlignment);
			snapshot->gradientStopArray.InsertRange((int)((alignedOffset - snapshot->gradientStopArray.GetByteSize()) / sizeof(FUIGradientStop)));
			snapshot->gradientStopSplits.Insert({ .StartOffset = alignedOffset, .ByteSize = drawList->gradientStopArray.GetByteSize() });
			snapshot->gradientStopArray.Insert(drawList->gradientStopArray.GetData(), (int)drawList->gradientStopArray.GetCount());
		}

		u32 drawCmdSplitCount = layer->GetSplitPointCount();

		SizeT cmdBase = snapshot->drawCmdSplits.Last().StartOffset / sizeof(FUIDrawCmd);
		SizeT prevSplit = 0;

		FMat4 layerGlobalMatrix = layer->GetGlobalTransform().ToMatrix4x4();
		snapshot->transformMatricesPerLayer.Insert(layerGlobalMatrix);

		for (u32 i = 0; i < drawCmdSplitCount; i++)
		{
			SizeT sp = layer->GetSplitPoint(i);

			FRenderPass rp1 = {
				//.renderTarget = nullptr,
				.LayerIndex = (SizeT)layerIndex,
				.DrawCmdStartIndex = cmdBase + prevSplit,
				.DrawCmdCount = sp - prevSplit   // excludes the placeholder at sp
			};

			if (rp1.DrawCmdCount > 0)
			{
				// Emit render pass for this layer's cmds before the split point
				snapshot->renderPassArray.Insert(rp1);

				prevSplit = sp; // There is no "dummy" draw command for split points.
			}

			// Child's index in the split arrays = current count before it inserts
			CompositeLayer(snapshot, layer->GetChild(i), (int)snapshot->vertexSplits.GetCount());
		}

		FRenderPass rp2 = {
			//.renderTarget = nullptr,
			.LayerIndex = (SizeT)layerIndex,
			.DrawCmdStartIndex = cmdBase + prevSplit,
			.DrawCmdCount = snapshot->drawCmdSplits[layerIndex].ByteSize / sizeof(FUIDrawCmd) - prevSplit
		};

		if (rp2.DrawCmdCount == 0)
			return;

		// Final segment after the last split (or the whole thing if no splits)
		snapshot->renderPassArray.Insert(rp2);
	}
} // namespace Fusion

#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	FSurface::FSurface(FName name) : Super(MoveTemp(name))
	{
		
	}

	Ref<FStyleSheet> FSurface::GetStyleSheet() const
	{
        if (m_StyleSheet)
            return m_StyleSheet;

        if (Ref<FSurface> parent = GetParentSurface())
        {
            return parent->GetStyleSheet();
        }

        if (Ref<FApplicationInstance> application = GetApplication())
        {
            return application->GetStyleSheet();
        }

        return nullptr;
	}

	FWidget* FSurface::HitTestWidget(FVec2 pos, FWidget* widget)
	{
        if (widget == nullptr)
        {
            widget = m_RootWidget.Get();
            if (!widget)
                return nullptr;
        }

        if (widget->Excluded() || widget->IsFaulted() || !widget->Visible())
            return nullptr;

        // Broad-phase: pos is in widget's own layer space, cachedLayerSpaceAABB is too
        if (!widget->m_CachedLayerSpaceAABB.Contains(pos))
            return nullptr;

        // Walk children last-to-first (last = painted on top = check first)
        for (int i = (int)widget->GetChildCount() - 1; i >= 0; i--)
        {
            FWidget* child = widget->GetChildAt(i).Get();
            if (!child) continue;

            FVec2 childPos;
            if (child->IsPaintBoundary())
            {
                // Child layer space = parent widget's local space numerically.
                // Convert: parent layer pos → child layer pos via parent widget's transform inverse.
                childPos = widget->m_CachedLayerSpaceTransform.Inverse().TransformPoint(pos);
            }
            else
            {
                childPos = pos;
            }

            if (FWidget* hit = HitTestWidget(childPos, child))
                return hit;
        }

        // Exact self hit test — convert layer pos → widget local space
        FVec2 localPos = widget->m_CachedLayerSpaceTransform.Inverse().TransformPoint(pos);
        if (widget->SelfHitTest(localPos))
            return widget;

        return nullptr;
	}

	void FSurface::DispatchSurfaceUnfocusEvent()
	{

	}

	void FSurface::DispatchSurfaceFocusEvent()
	{

	}

    static void BuildHoverStack(FWidget* leaf, FArray<Ref<FWidget>>& outStack)
    {
        FWidget* w = leaf;
        while (w)
        {
            outStack.Add(w);
            w = w->GetParentWidget().Get();
        }
    }

	void FSurface::DispatchMouseEvents()
	{
        ZoneScoped;

        Ref<FApplicationInstance> application = GetApplication();
        if (!application)
            return;

        // - Mouse Pos

        FVec2 screenMousePos = application->GetScreenMousePos();
        FVec2 prevScreenMousePos = application->GetPrevScreenMousePos();
        FVec2 wheelDelta = application->GetMouseWheelDelta();

        FVec2 surfaceMousePos = ScreenToSurfacePoint(screenMousePos);
        FVec2 prevSurfaceMousePos = ScreenToSurfacePoint(prevScreenMousePos);
        FVec2 surfaceMouseDelta = surfaceMousePos - prevSurfaceMousePos;

        // - Mouse Buttons

        EMouseButtonMask curButtonMask = EMouseButtonMask::None;
        if (application->IsMouseButtonHeld(EMouseButton::Left))
        {
            curButtonMask |= EMouseButtonMask::Left;
        }
        if (application->IsMouseButtonHeld(EMouseButton::Right))
        {
            curButtonMask |= EMouseButtonMask::Right;
        }
        if (application->IsMouseButtonHeld(EMouseButton::Middle))
        {
            curButtonMask |= EMouseButtonMask::Middle;
        }
        if (application->IsMouseButtonHeld(EMouseButton::Button4))
        {
            curButtonMask |= EMouseButtonMask::Button4;
        }
        if (application->IsMouseButtonHeld(EMouseButton::Button5))
        {
            curButtonMask |= EMouseButtonMask::Button5;
        }

        // - Key Modifiers

        EKeyModifier keyModifiers = application->GetKeyModifiers();

        bool mouseInSurface = FRect::FromSize(FVec2(), GetAvailableSize()).Contains(surfaceMousePos);

        Ref<FWidget> captured = capturedWidget.Lock();

        Ref<FWidget> hitWidget = mouseInSurface ? HitTestWidget(surfaceMousePos) : nullptr;

        Ref<FWidget> hoveredWidget = captured.IsValid() ? captured.Get() : hitWidget;

        FArray<Ref<FWidget>> newHoverStack;
        BuildHoverStack(hoveredWidget.Get(), newHoverStack);

        // Mouse Leave
        for (WeakRef<FWidget>& weakWidget : hoveredWidgetStack)
        {
            Ref<FWidget> widget = weakWidget.Lock();
            if (!widget || newHoverStack.Contains(widget.Get()))
                continue;

            FMouseEvent event{};
            event.Type = EEventType::MouseLeave;
            event.Sender = widget;
            event.MousePosition = surfaceMousePos;
            event.PrevMousePosition = prevSurfaceMousePos;
            event.WheelDelta = wheelDelta;
            event.bIsInside = false;
            event.KeyModifiers = keyModifiers;

            FWidget* parent = widget->GetParentWidget().Get();

            FUSION_TRY
            {
                widget->OnMouseLeave(event);
            }
            FUSION_CATCH (const FException& e)
            {
                widget->SetFaulted();
                FUSION_LOG_ERROR("Widget", "FException in {}::OnMouseLeave(): {}\n{}", widget->GetClassName(), e.what(), e.GetStackTraceString(true));
            }
        }

        // Mouse Enter
        for (int i = newHoverStack.Size() - 1; i >= 0; i--)
        {
            Ref<FWidget> widget = newHoverStack[i];

            bool wasHovered = hoveredWidgetStack.Contains(widget);
            if (wasHovered)
                continue;

            FMouseEvent event{};
            event.Type = EEventType::MouseEnter;
            event.Sender = widget;
            event.MousePosition = surfaceMousePos;
            event.PrevMousePosition = prevSurfaceMousePos;
            event.bIsInside = true;
            event.KeyModifiers = keyModifiers;

            Ref<FWidget> parent = widget->GetParentWidget();

            FUSION_TRY
            {
                widget->OnMouseEnter(event);
            }
            FUSION_CATCH (const FException& e)
            {
                widget->SetFaulted();
                FUSION_LOG_ERROR("Widget", "FException in {}::OnMouseEnter(): {}\n{}", widget->GetClassName(), e.what(), e.GetStackTraceString(true));
            }
        }

        hoveredWidgetStack.Clear();
        for (Ref<FWidget> widget : newHoverStack)
            hoveredWidgetStack.Add(widget);

        if (hoveredWidget)
        {
            constexpr auto epsilon = std::numeric_limits<float>::epsilon();

            // Mouse Move
            if (abs(surfaceMouseDelta.x) >= epsilon || abs(surfaceMouseDelta.y) >= epsilon)
            {
                FMouseEvent event{};
                event.Type = EEventType::MouseMove;
                event.Sender = hoveredWidget;
                event.MousePosition = surfaceMousePos;
                event.PrevMousePosition = prevSurfaceMousePos;
                event.Buttons = curButtonMask;
                event.bIsInside = true;
                event.KeyModifiers = keyModifiers;

                FWidget* parent = hoveredWidget->GetParentWidget().Get();

                FUSION_TRY
                {
                    FEventReply reply = hoveredWidget->OnMouseMove(event);
                    ProcessReply(hoveredWidget, reply);
                }
                FUSION_CATCH (const FException& e)
                {
                    hoveredWidget->SetFaulted();
                    FUSION_LOG_ERROR("Widget", "FException in {}::OnMouseMove(): {}\n{}", hoveredWidget->GetClassName(), e.what(), e.GetStackTraceString(true));
                }
            }

            // Mouse Wheel
            if (abs(wheelDelta.x) >= epsilon || abs(wheelDelta.y) >= epsilon)
            {
                for (WeakRef<FWidget>& weakWidget : hoveredWidgetStack)
                {
                    Ref<FWidget> widget = weakWidget.Lock();
                    if (!widget) continue;

                    FMouseEvent event{};
                    event.Type = EEventType::MouseWheel;
                    event.Sender = hoveredWidget;
                    event.MousePosition = surfaceMousePos;
                    event.PrevMousePosition = prevSurfaceMousePos;
                    event.WheelDelta = wheelDelta;
                    event.Buttons = curButtonMask;
                    event.bIsInside = true;
                    event.KeyModifiers = keyModifiers;

                    FWidget* parent = widget->GetParentWidget().Get();

                    FUSION_TRY
                    {
                        FEventReply reply = widget->OnMouseWheel(event);
                        ProcessReply(widget.Get(), reply);
                        if (reply.IsHandled())
                            break;
                    }
                    FUSION_CATCH (const FException& e)
                    {
                        widget->SetFaulted();
                        FUSION_LOG_ERROR("Widget", "FException in {}::OnMouseWheel(): {}\n{}", widget->GetClassName(), e.what(), e.GetStackTraceString(true));
                    }
                }
            }
        }

        constexpr EMouseButton kButtons[] = {
            EMouseButton::Left, EMouseButton::Right, EMouseButton::Middle,
            EMouseButton::Button4, EMouseButton::Button5
        };
        constexpr EMouseButtonMask kMasks[] = {
            EMouseButtonMask::Left, EMouseButtonMask::Right, EMouseButtonMask::Middle,
            EMouseButtonMask::Button4, EMouseButtonMask::Button5
        };

        // - Mouse Down/Up

        for (int i = 0; i < 5; i++)
        {
            if (application->IsMouseButtonDown(kButtons[i]))
            {
                Ref<FWidget> target = captured ? captured.Get() : hitWidget;
                if (target)
                {
                    FMouseEvent downEvent{};
                    downEvent.Type = EEventType::MouseButtonDown;
                    downEvent.Sender = target;
                    downEvent.MousePosition = surfaceMousePos;
                    downEvent.PrevMousePosition = prevSurfaceMousePos;
                    downEvent.Buttons = kMasks[i];
                    downEvent.bIsInside = true;
                    downEvent.bIsDoubleClick = application->GetMouseButtonClicks(kButtons[i]) == 2;
                    downEvent.KeyModifiers = keyModifiers;

                    // Bubble up until handled
                    Ref<FWidget> current = target;
                    while (current)
                    {
                        downEvent.Sender = current;
                        FWidget* parent = current->GetParentWidget().Get();

                        FUSION_TRY
                        {
                            FEventReply reply = current->OnMouseButtonDown(downEvent);
                            ProcessReply(current, reply);
                            if (reply.IsHandled())
                                break;
                        }
                        FUSION_CATCH (const FException& e)
                        {
                            current->SetFaulted();
                            FUSION_LOG_ERROR("Widget", "FException in {}::OnMouseButtonDown(): {}\n{}", current->GetClassName(), e.what(), e.GetStackTraceString(true));

                            if (parent)
                            {
                                parent->MarkLayoutDirty();
                                parent->MarkPaintDirty();
                            }
                            else if (m_RootWidget)
                            {
                                m_RootWidget->MarkLayoutDirty();
                                m_RootWidget->MarkPaintDirty();
                            }
                        }

                        current = parent;
                    }

                    pressedWidgetPerButton[i] = target;
                }
            }

            if (application->IsMouseButtonUp(kButtons[i]))
            {
                Ref<FWidget> pressed = pressedWidgetPerButton[i].Lock();
                FWidget* upTarget = captured ? captured.Get()
                    : pressed ? pressed.Get()
                    : nullptr;

                if (upTarget)
                {
                    FMouseEvent upEvent{};
                    upEvent.Type = EEventType::MouseButtonUp;
                    upEvent.MousePosition = surfaceMousePos;
                    upEvent.PrevMousePosition = prevSurfaceMousePos;
                    upEvent.Buttons = kMasks[i];
                    upEvent.bIsInside = (upTarget == hitWidget.Get());
                    upEvent.KeyModifiers = keyModifiers;

                    // Bubble up from the pressed/captured widget
                    FWidget* current = upTarget;
                    while (current)
                    {
                        upEvent.Sender = current;
                        FWidget* parent = current->GetParentWidget().Get();

                        FUSION_TRY
                        {
                            FEventReply reply = current->OnMouseButtonUp(upEvent);
                            ProcessReply(current, reply);
                            if (reply.IsHandled())
                                break;
                        }
                        FUSION_CATCH (const FException& e)
                        {
                            current->SetFaulted();
                            FUSION_LOG_ERROR("Widget", "FException in {}::OnMouseButtonUp(): {}\n{}", current->GetClassName(), e.what(), e.GetStackTraceString(true));
                        }

                        current = parent;
                    }
                }

                pressedWidgetPerButton[i] = nullptr;
            }
        }

        Ref<FWidget> nextFocus = nextFocusWidget.Lock();
        Ref<FWidget> curFocus = curFocusedWidget.Lock();

        if (nextFocus != curFocus)
        {
            if (curFocus)
            {
                FFocusEvent lostEvent{};
                lostEvent.Type = EEventType::FocusChanged;
                lostEvent.bGotFocus = false;
                lostEvent.FocusedWidget = nextFocus;
                lostEvent.Sender = curFocus;

                FWidget* parent = curFocus->GetParentWidget().Get();

                FUSION_TRY
                {
                    curFocus->OnFocusChanged(lostEvent);
                }
                FUSION_CATCH (const FException& e)
                {
                    curFocus->SetFaulted();
                    FUSION_LOG_ERROR("Widget", "FException in {}::OnFocusChanged(): {}\n{}", curFocus->GetClassName(), e.what(), e.GetStackTraceString(true));
                }
            }

            if (nextFocus)
            {
                FFocusEvent gotEvent{};
                gotEvent.Type = EEventType::FocusChanged;
                gotEvent.bGotFocus = true;
                gotEvent.FocusedWidget = nextFocus;
                gotEvent.Sender = nextFocus;

                FWidget* parent = nextFocus->GetParentWidget().Get();

                FUSION_TRY
                {
                    nextFocus->OnFocusChanged(gotEvent);
                }
                FUSION_CATCH (const FException& e)
                {
                    nextFocus->SetFaulted();
                    FUSION_LOG_ERROR("Widget", "FException in {}::OnFocusChanged(): {}\n{}", nextFocus->GetClassName(), e.what(), e.GetStackTraceString(true));
                }
            }

            curFocusedWidget = nextFocus;
            nextFocusWidget = nullptr;
        }
	}

	void FSurface::DispatchKeyEvents()
	{

	}

	void FSurface::ProcessReply(Ref<FWidget> Sender, const FEventReply& reply)
	{

	}

	void FSurface::Initialize()
	{
		Ref<FApplicationInstance> application = GetApplication();
		FUSION_ASSERT(application.IsValid(), "Application Instance not found!");

		m_RenderCapabilities = application->GetRenderCapabilities();

		m_LayerTree = NewObject<FLayerTree>(this);
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

		for (int i = (int)m_PendingLayoutRoots.Size() - 1; i >= 0; i--)
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

		IPtr<FRenderSnapshot> snapshot = new FRenderSnapshot();

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

	void FSurface::OnSurfaceResize()
	{
		MarkRootLayoutDirty();
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

		//const SizeT constantBufferAlignment = m_RenderCapabilities.MinConstantBufferOffsetAlignment;
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

			FUIRenderPass rp1 = {
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

		FUIRenderPass rp2 = {
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

	void FSurface::RefreshStyleRecursively(Ref<FWidget> widget)
	{
        if (widget == nullptr)
            widget = m_RootWidget;

        if (widget == nullptr)
            return;

        widget->RefreshStyleRecursively();
	}

} // namespace Fusion

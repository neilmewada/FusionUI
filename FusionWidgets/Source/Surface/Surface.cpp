#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	FSurface::FSurface(FName name) : Super(MoveTemp(name))
	{
		
	}

	Ref<FTheme> FSurface::GetTheme() const
	{
        if (m_Theme)
            return m_Theme;

        if (Ref<FSurface> parent = GetParentSurface())
        {
            return parent->GetTheme();
        }

        if (Ref<FApplicationInstance> application = GetApplication())
        {
            return application->GetTheme();
        }

        return nullptr;
	}

	FWidget* FSurface::HitTestWidget(FVec2 pos, FWidget* widget)
	{
        ZoneScoped;

        if (widget == nullptr)
        {
            widget = m_RootWidget.Get();
            if (!widget)
                return nullptr;
        }

        if (widget->Excluded() || widget->IsFaulted() || !widget->Visible())
            return nullptr;

        // Broad-phase: pos is in widget's own layer space, cachedLayerSpaceAABB is too.
        if (!widget->m_CachedLayerSpaceAABB.Contains(pos))
            return nullptr;

        // convert layer pos → widget local space
        const FVec2 localPos = widget->m_CachedLayerSpaceTransform.Inverse().TransformPoint(pos);
        const bool selfHit = widget->SelfHitTest(localPos);

        if (selfHit && !widget->ShouldHitTestChildren(localPos))
        {
            return widget;
        }

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

        // Exact self hit test
        if (selfHit)
            return widget;

        return nullptr;
	}

	void FSurface::DispatchSurfaceUnfocusEvent()
	{

	}

	void FSurface::DispatchSurfaceFocusEvent()
	{

	}

    static void BuildHoverStack(FWidget* leaf, TArray<Ref<FWidget>>& outStack)
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

        Ref<FWidget> captured = m_CapturedWidget.Lock();

        Ref<FWidget> hitWidget = mouseInSurface ? HitTestWidget(surfaceMousePos) : nullptr;

        Ref<FWidget> hoveredWidget = captured.IsValid() ? captured : hitWidget;

        TArray<Ref<FWidget>> newHoverStack;
        BuildHoverStack(hoveredWidget.Get(), newHoverStack);

        UpdateCursor(hoveredWidget, surfaceMousePos);

        // Mouse Leave
        for (WeakRef<FWidget>& weakWidget : m_HoveredWidgetStack)
        {
            Ref<FWidget> widget = weakWidget.Lock();
            if (!widget || newHoverStack.Contains(widget))
                continue;

            FMouseEvent event{};
            event.Type = EEventType::MouseLeave;
            event.Sender = widget;
            event.Target = widget;
            event.MousePosition = surfaceMousePos;
            event.PrevMousePosition = prevSurfaceMousePos;
            event.WheelDelta = wheelDelta;
            event.bIsInside = false;
            event.KeyModifiers = keyModifiers;

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

            bool wasHovered = m_HoveredWidgetStack.Contains(widget);
            if (wasHovered)
                continue;

            FMouseEvent event{};
            event.Type = EEventType::MouseEnter;
            event.Sender = widget;
            event.Target = widget;
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

        m_HoveredWidgetStack.Clear();
        for (Ref<FWidget> widget : newHoverStack)
            m_HoveredWidgetStack.Add(widget);

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

                for (WeakRef<FWidget>& weakWidget : m_HoveredWidgetStack)
                {
                    Ref<FWidget> widget = weakWidget.Lock();
                    if (!widget) continue;

                    event.Sender = widget;
                    event.Target = hoveredWidget;

                    FUSION_TRY
                    {
                        FEventReply reply = widget->OnMouseMove(event);
                        ProcessReply(widget, reply);
                        if (reply.IsHandled())
                            break;
                    }
                    FUSION_CATCH (const FException& e)
                    {
                        widget->SetFaulted();
                        FUSION_LOG_ERROR("Widget", "FException in {}::OnMouseMove(): {}\n{}", widget->GetClassName(), e.what(), e.GetStackTraceString(true));
                    }
                }
            }

            // Mouse Wheel
            if (abs(wheelDelta.x) >= epsilon || abs(wheelDelta.y) >= epsilon)
            {
                for (WeakRef<FWidget>& weakWidget : m_HoveredWidgetStack)
                {
                    Ref<FWidget> widget = weakWidget.Lock();
                    if (!widget) continue;

                    FMouseEvent event{};
                    event.Type = EEventType::MouseWheel;
                    event.Sender = widget;
                    event.Target = hoveredWidget;
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
                        ProcessReply(widget, reply);
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
                if (kButtons[i] == EMouseButton::Left || kButtons[i] == EMouseButton::Right)
                {
                    // Clear focus on every left/right click. The widget will call FocusSelf() if needed.
	                m_NextFocusWidget = nullptr;
                }

                Ref<FWidget> target = captured ? captured : hitWidget;
                if (target)
                {
                    FMouseEvent downEvent{};
                    downEvent.Type = EEventType::MouseButtonDown;
                    downEvent.Sender = target;
                    downEvent.MousePosition = surfaceMousePos;
                    downEvent.PrevMousePosition = prevSurfaceMousePos;
                    downEvent.Buttons = kMasks[i];
                    downEvent.bIsInside  = true;
                    downEvent.ClickCount = application->GetMouseButtonClicks(kButtons[i]);
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

                    m_PressedWidgetPerButton[i] = target;
                }
            }

            if (application->IsMouseButtonUp(kButtons[i]))
            {
                Ref<FWidget> pressed = m_PressedWidgetPerButton[i].Lock();
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

                m_PressedWidgetPerButton[i] = nullptr;
            }
        }

        ApplyPendingFocus();
	}

	void FSurface::DispatchKeyEvents()
	{
		ZoneScoped;

		Ref<FApplicationInstance> application = GetApplication();
		if (!application)
			return;

		Ref<FWidget> focusedWidget = m_CurFocusedWidget.Lock();
		if (!focusedWidget)
			return;

		IFPlatformBackend* platform = application->GetPlatformBackend();
		EKeyModifier modifiers = application->GetKeyModifiers();

		// KeyDown
		for (EKeyCode key : platform->GetKeysDownThisTick())
		{
			FKeyEvent event{};
			event.Type = EEventType::KeyDown;
			event.Key = key;
			event.Modifiers = modifiers;

			FWidget* current = focusedWidget.Get();
			while (current)
			{
				event.Sender = current;
				FWidget* parent = current->GetParentWidget().Get();

				FUSION_TRY
				{
					FEventReply reply = current->OnKeyDown(event);
					ProcessReply(current, reply);
					if (reply.IsHandled())
						break;
				}
				FUSION_CATCH (const FException& e)
				{
					current->SetFaulted();
					FUSION_LOG_ERROR("Widget", "FException in {}::OnKeyDown(): {}\n{}", current->GetClassName(), e.what(), e.GetStackTraceString(true));
					break;
				}

				current = parent;
			}
		}

		// KeyUp
		for (EKeyCode key : platform->GetKeysUpThisTick())
		{
			FKeyEvent event{};
			event.Type = EEventType::KeyUp;
			event.Key = key;
			event.Modifiers = modifiers;

			FWidget* current = focusedWidget.Get();
			while (current)
			{
				event.Sender = current;
				FWidget* parent = current->GetParentWidget().Get();

				FUSION_TRY
				{
					FEventReply reply = current->OnKeyUp(event);
					ProcessReply(current, reply);
					if (reply.IsHandled())
						break;
				}
				FUSION_CATCH (const FException& e)
				{
					current->SetFaulted();
					FUSION_LOG_ERROR("Widget", "FException in {}::OnKeyUp(): {}\n{}", current->GetClassName(), e.what(), e.GetStackTraceString(true));
					break;
				}

				current = parent;
			}
		}

		// TextInput
		FString textInput = platform->GetTextInputThisTick();
		if (!textInput.Empty())
		{
			FTextInputEvent event{};
			event.Type = EEventType::TextInput;
			event.Text = MoveTemp(textInput);

			FWidget* current = focusedWidget.Get();
			while (current)
			{
				event.Sender = current;
				FWidget* parent = current->GetParentWidget().Get();

				FUSION_TRY
				{
					FEventReply reply = current->OnTextInput(event);
					ProcessReply(current, reply);
					if (reply.IsHandled())
						break;
				}
				FUSION_CATCH (const FException& e)
				{
					current->SetFaulted();
					FUSION_LOG_ERROR("Widget", "FException in {}::OnTextInput(): {}\n{}", current->GetClassName(), e.what(), e.GetStackTraceString(true));
					break;
				}

				current = parent;
			}
		}

		ApplyPendingFocus();
	}

	void FSurface::ProcessReply(Ref<FWidget> sender, const FEventReply& reply)
	{
        Ref<FApplicationInstance> application = m_Application.Lock();
        if (!application)
            return;

        switch (reply.GetFocusOp())
        {
        case FEventReply::FocusOp::Self:
            m_NextFocusWidget = sender;
            m_bNextFocusFromKeyboard = false;
            break;
        case FEventReply::FocusOp::Next:
            m_NextFocusWidget = FindNextFocusable(m_CurFocusedWidget.Lock(), false);
            m_bNextFocusFromKeyboard = true;
            break;
        case FEventReply::FocusOp::Prev:
            m_NextFocusWidget = FindNextFocusable(m_CurFocusedWidget.Lock(), true);
            m_bNextFocusFromKeyboard = true;
            break;
        default:
            break;
        }

        switch (reply.GetMouseCaptureOp())
        {
        case FEventReply::MouseCaptureOp::Capture:
        	m_CapturedWidget = sender;
            m_CapturedWidgetCursorOverride = !reply.GetCursorOverride().IsInherited();
            if (m_CapturedWidgetCursorOverride)
            {
	            application->PushCursorOverride(reply.GetCursorOverride());
            }
        	break;
        case FEventReply::MouseCaptureOp::Release:
            if (m_CapturedWidget == sender)
            {
                if (m_CapturedWidgetCursorOverride)
                {
                    application->PopCursorOverride();
                    m_CapturedWidgetCursorOverride = false;
                }
	            m_CapturedWidget = nullptr;
            }
        	break;
        default:
        	break;
        }
	}

	Ref<FWidget> FSurface::FindNextFocusable(Ref<FWidget> current, bool reverse)
	{
        TArray<Ref<FWidget>> focusable;

        std::function<void(Ref<FWidget>)> collect = [&](Ref<FWidget> widget)
        {
            if (!widget || widget->IsHidden() || widget->IsExcluded() || widget->TestStyleState(EStyleState::Disabled))
                return;

            if (widget->IsFocusable())
                focusable.Add(widget);

            u32 count = widget->GetChildCount();
            for (u32 i = 0; i < count; i++)
                collect(widget->GetChildAt(i));
        };

        collect(m_RootWidget);

        if (focusable.Empty())
            return nullptr;

        if (!current)
            return reverse ? focusable.Last() : focusable[0];

        int idx = -1;
        for (int i = 0; i < (int)focusable.Size(); i++)
        {
            if (focusable[i] == current)
            {
                idx = i;
                break;
            }
        }

        if (idx == -1)
            return reverse ? focusable.Last() : focusable[0];

        int next = reverse
            ? (idx - 1 + (int)focusable.Size()) % (int)focusable.Size()
            : (idx + 1) % (int)focusable.Size();
        return focusable[next];
	}

	void FSurface::ApplyPendingFocus()
	{
        Ref<FWidget> nextFocus = m_NextFocusWidget.Lock();
        Ref<FWidget> curFocus  = m_CurFocusedWidget.Lock();

        if (nextFocus == curFocus)
            return;

        if (curFocus)
        {
            FFocusEvent lostEvent{};
            lostEvent.Type         = EEventType::FocusChanged;
            lostEvent.bGotFocus    = false;
            lostEvent.FocusedWidget = nextFocus;
            lostEvent.Sender       = curFocus;

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
            gotEvent.Type          = EEventType::FocusChanged;
            gotEvent.bGotFocus     = true;
            gotEvent.bFromKeyboard = m_bNextFocusFromKeyboard;
            gotEvent.FocusedWidget = nextFocus;
            gotEvent.Sender        = nextFocus;

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

        m_CurFocusedWidget = nextFocus;
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

        if (Ref<FLayer> overlayLayer = m_LayerTree->GetOverlayLayer())
        {
            if (overlayLayer->m_NeedsCompositing)
            {
	            CompositeLayer(snapshot, overlayLayer, (int)snapshot->vertexSplits.GetCount());
            }
        }

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

		FUIDrawList* drawList = layer->GetDrawList();

		snapshot->vertexSplits.Insert({ .StartOffset = snapshot->vertexArray.GetByteSize(), .ByteSize = drawList->vertexArray.GetByteSize() });
		snapshot->vertexArray.InsertRange(drawList->vertexArray.GetData(), (int)drawList->vertexArray.GetCount());

		snapshot->indexSplits.Insert({ .StartOffset = snapshot->indexArray.GetByteSize(), .ByteSize = drawList->indexArray.GetByteSize() });
		snapshot->indexArray.InsertRange(drawList->indexArray.GetData(), (int)drawList->indexArray.GetCount());

		snapshot->drawItemSplits.Insert({ .StartOffset = snapshot->drawItemArray.GetByteSize(), .ByteSize = drawList->drawItemArray.GetByteSize() });
		snapshot->drawItemArray.InsertRange(drawList->drawItemArray.GetData(), (int)drawList->drawItemArray.GetCount());

		snapshot->drawCmdSplits.Insert({ .StartOffset = snapshot->drawCmdArray.GetByteSize(), .ByteSize = drawList->drawCmdArray.GetByteSize() });
		snapshot->drawCmdArray.InsertRange(drawList->drawCmdArray.GetData(), (int)drawList->drawCmdArray.GetCount());

		snapshot->clipRectSplits.Insert({ .StartOffset = snapshot->clipRectArray.GetByteSize(), .ByteSize = drawList->clipRectArray.GetByteSize() });
		snapshot->clipRectArray.InsertRange(drawList->clipRectArray.GetData(), (int)drawList->clipRectArray.GetCount());

		snapshot->gradientStopSplits.Insert({ .StartOffset = snapshot->gradientStopArray.GetByteSize(), .ByteSize = drawList->gradientStopArray.GetByteSize() });
		snapshot->gradientStopArray.InsertRange(drawList->gradientStopArray.GetData(), (int)drawList->gradientStopArray.GetCount());
		

		u32 drawCmdSplitCount = layer->GetSplitPointCount();

		SizeT cmdBase = snapshot->drawCmdSplits.Last().StartOffset / sizeof(FUIDrawCmd);
		SizeT prevSplit = 0;

		FMat4 layerGlobalMatrix = layer->GetGlobalTransform().ToMat4();
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

	void FSurface::UpdateCursor(Ref<FWidget> hoveredWidget, FVec2 surfaceMousePos)
	{
        FCursor resolved = FCursor::System(ESystemCursor::Arrow); // default fallback cursor

        Ref<FWidget> current = hoveredWidget;

        // Walk up from hovered widget until a non-Inherited cursor is found
        while (current != nullptr)
        {
            FVec2 localPos = current->GetGlobalTransform().Inverse().TransformPoint(surfaceMousePos);
            FCursor c = current->GetActiveCursorAt(localPos);
            if (!c.IsInherited())
            {
                resolved = c;
                break;
            }
            current = current->GetParentWidget();
        }

        GetApplication()->SetActiveCursor(resolved);
	}
} // namespace Fusion

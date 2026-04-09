#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	f32 FApplicationInstance::GetDpiScaleForWindow(FWindowHandle windowHandle)
	{
		return m_PlatformBackend->GetDpiScaleForWindow(windowHandle);
	}

	FVec2i FApplicationInstance::GetWindowSizeInPixels(FWindowHandle windowHandle)
	{
		return m_PlatformBackend->GetWindowSizeInPixels(windowHandle);
	}

	bool FApplicationInstance::Initialize(const FApplicationInstanceDesc& desc)
	{
		m_PlatformBackend = desc.PlatformBackend;
		
#if FUSION_USE_SDL3
		if (!m_PlatformBackend)
		{
			m_PlatformBackendAllocated = true;
			m_PlatformBackend = new FSDL3PlatformBackend();
		}
#endif

		if (!m_PlatformBackend)
		{
			FUSION_LOG_ERROR("Backend", "No platform backend specified and no default available.");
			return false;
		}

		m_RenderBackend = desc.RenderBackend;

#if FUSION_USE_VULKAN
		if (!m_RenderBackend)
		{
			m_RenderBackendAllocated = true;
			m_RenderBackend = new Vulkan::FVulkanRenderBackend(m_PlatformBackend);
		}
#endif

		if (!m_RenderBackend)
		{
			FUSION_LOG_ERROR("Backend", "No render backend specified and no default available.");
			return false;
		}

		m_InstanceHandle = m_PlatformBackend->InitializeInstance();

		if (m_InstanceHandle.IsNull())
		{
			FUSION_LOG_ERROR("Backend", "Failed to initialize platform backend instance.");
			return false;
		}

		m_PlatformBackend->SetInstanceEventSink(m_InstanceHandle, this);

		m_RenderBackend->InitializeInstance(m_InstanceHandle);

		m_RenderCapabilities = m_RenderBackend->GetRenderCapabilities();
		FUSION_ASSERT(m_RenderCapabilities.MinStructuredBufferOffsetAlignment > 0, "Invalid value for MinStructuredBufferOffsetAlignment.");

		m_FontAtlas = NewObject<FFontAtlas>(this, this);
		m_RenderBackend->CreateLayeredAtlas(true, 2048, 4);

		m_FontAtlas->Initialize();

		return true;
	}

	void FApplicationInstance::Shutdown()
	{
		m_FontAtlas->Shutdown();
		m_FontAtlas = nullptr;

		if (m_RenderBackend)
		{
			m_RenderBackend->ShutdownInstance(m_InstanceHandle);
		}

		if (m_PlatformBackend)
		{
			m_PlatformBackend->ShutdownInstance(m_InstanceHandle);
		}

		m_InstanceHandle = FInstanceHandle::NullValue;

		if (m_PlatformBackendAllocated)
		{
			delete m_PlatformBackend; m_PlatformBackend = nullptr;
		}

		if (m_RenderBackendAllocated)
		{
			delete m_RenderBackend; m_RenderBackend = nullptr;
		}

		m_PlatformBackendAllocated = m_RenderBackendAllocated = false;
	}

	void FApplicationInstance::Tick()
	{
		ZoneScoped;

		auto curTime = std::chrono::steady_clock::now();
		m_DeltaTime = std::chrono::duration<f32>(curTime - m_PreviousTime).count();

		// - Animation -

		for (auto& [ownerUuid, animationsBySlot] : m_AnimationSlotsByWidget)
		{
			for (auto& [slot, animation] : animationsBySlot)
			{
				if (!animation->IsOwnerValid())
				{
					m_AnimationsToDestroy.Add({ ownerUuid, slot });
					continue;
				}

				animation->Tick(m_DeltaTime);

				if (animation->GetState() == EAnimationState::Completed ||
					animation->GetState() == EAnimationState::Idle)
				{
					m_AnimationsToDestroy.Add({ ownerUuid, slot });
				}
			}
		}

		m_PrevScreenMousePos = m_ScreenMousePos;
		m_ScreenMousePos = m_PlatformBackend->GetGlobalMousePosition();
		m_WheelDelta = m_PlatformBackend->GetMouseWheelDelta();

		if (m_IsFirstTick)
		{
			m_IsFirstTick = false;
			m_PrevScreenMousePos = m_ScreenMousePos;
		}

		Ref<FSurface> curFocus = m_CurFocusSurface.Lock();
		Ref<FSurface> focus = m_FocusSurface.Lock();

		// TODO: Fix Surface focus logic

		if (focus != curFocus)
		{
			if (curFocus) curFocus->DispatchSurfaceUnfocusEvent();
			if (focus)    focus->DispatchSurfaceFocusEvent();
		}

		if (focus)
		{
			focus->DispatchMouseEvents();  // surface resolves coords + runs state machine
			focus->DispatchKeyEvents();    // routes to curFocusedWidget
		}

		m_CurFocusSurface = m_FocusSurface;

		for (Ref<FSurface> surface : m_Surfaces)
		{
			surface->TickSurface();
		}

		m_PreviousTime = MoveTemp(curTime);
	}

	Ref<FNativeSurface> FApplicationInstance::CreateNativeSurfaceForWindow(FWindowHandle window)
	{
		Ref<FNativeSurface> nativeSurface = new FNativeSurface(window);
		AttachSubobject(nativeSurface);

		nativeSurface->m_Application = Ref(this);

		m_NativeSurfacesByWindow[window] = nativeSurface;

		m_Surfaces.Add(nativeSurface);

		nativeSurface->Initialize();

		return nativeSurface;
	}

	void FApplicationInstance::FocusSurface(Ref<FSurface> surface)
	{
		m_FocusSurface = surface;
	}

	void FApplicationInstance::UnfocusSurface(Ref<FSurface> surface)
	{
		if (Ref<FSurface> focus = m_FocusSurface.Lock())
		{
			if (surface == focus)
			{
				m_FocusSurface = nullptr;
			}
		}
	}

	FRenderTargetHandle FApplicationInstance::AcquireWindowRenderTarget(FWindowHandle window)
	{
		return m_RenderBackend->AcquireWindowRenderTarget(window);
	}

	void FApplicationInstance::ReleaseRenderTarget(FRenderTargetHandle renderTarget)
	{
		m_RenderBackend->ReleaseRenderTarget(renderTarget);
	}

	void FApplicationInstance::SubmitSnapshot(FRenderTargetHandle renderTarget, IntrusivePtr<FRenderSnapshot> snapshot)
	{
		m_RenderBackend->SubmitSnapshot(renderTarget, snapshot);
	}

	void FApplicationInstance::SetRootTheme(Ref<FTheme> theme)
	{
		if (m_RootTheme == theme)
			return;

		m_RootTheme = theme;

		RefreshStyleRecursively();
	}

	void FApplicationInstance::RefreshStyleRecursively()
	{
		for (Ref<FSurface> surface : m_Surfaces)
		{
			surface->RefreshStyleRecursively();
		}
	}

	void FApplicationInstance::PlayAnimation(Ref<FAnimation> animation, Ref<FObject> owner, FName slot)
	{
		if (!animation.IsValid() || !owner.IsValid())
			return;

		FUuid ownerUuid = owner->GetUuid();

		// Stop and destroy any existing animation on this slot
		auto ownerIt = m_AnimationSlotsByWidget.Find(ownerUuid);
		if (ownerIt != m_AnimationSlotsByWidget.end())
		{
			auto slotIt = ownerIt->second.Find(slot);
			if (slotIt != ownerIt->second.end())
			{
				slotIt->second->Stop();
				slotIt->second->BeginDestroy();
				ownerIt->second.Remove(slot);
			}
		}

		animation->Play();

		m_AnimationSlotsByWidget[ownerUuid][slot] = animation;
	}

	void FApplicationInstance::TerminateAnimation(Ref<FObject> owner, FName slot, bool complete)
	{
		if (!owner.IsValid())
			return;

		FUuid ownerUuid = owner->GetUuid();

		auto ownerIt = m_AnimationSlotsByWidget.Find(ownerUuid);
		if (ownerIt == m_AnimationSlotsByWidget.end())
			return;

		auto slotIt = ownerIt->second.Find(slot);
		if (slotIt == ownerIt->second.end())
			return;

		Ref<FAnimation>& animation = slotIt->second;

		if (complete)
			animation->Apply(1.0f);

		animation->Stop();
		m_AnimationsToDestroy.Add({ ownerUuid, slot });
	}

	void FApplicationInstance::TerminateAllAnimations(Ref<FObject> owner, bool complete)
	{
		if (!owner.IsValid())
			return;

		FUuid ownerUuid = owner->GetUuid();

		auto ownerIt = m_AnimationSlotsByWidget.Find(ownerUuid);
		if (ownerIt == m_AnimationSlotsByWidget.end())
			return;

		for (auto& [slot, animation] : ownerIt->second)
		{
			if (complete)
				animation->Apply(1.0f);

			animation->Stop();
			m_AnimationsToDestroy.Add({ ownerUuid, slot });
		}
	}

	void FApplicationInstance::OnWindowDestroyed(FWindowHandle window)
	{
		if (!m_NativeSurfacesByWindow.KeyExists(window))
			return;

		Ref<FNativeSurface> nativeSurface = m_NativeSurfacesByWindow[window];
		if (!nativeSurface)
			return;

		nativeSurface->Shutdown();

		m_NativeSurfacesByWindow.Remove(window);
		m_Surfaces.Remove(nativeSurface);
	}

	void FApplicationInstance::OnWindowResized(FWindowHandle window, u32 newWidth, u32 newHeight)
	{
		NotifyWindowResize(window);
	}

	void FApplicationInstance::OnWindowMaximized(FWindowHandle window)
	{
		NotifyWindowResize(window);
	}

	void FApplicationInstance::OnWindowMinimized(FWindowHandle window)
	{
		NotifyWindowResize(window);
	}

	void FApplicationInstance::OnWindowRestored(FWindowHandle window)
	{
		NotifyWindowResize(window);
	}

	void FApplicationInstance::OnWindowKeyboardFocusChanged(FWindowHandle window, bool gotFocus)
	{
		if (!gotFocus)
			return;

		auto it = m_NativeSurfacesByWindow.Find(window);
		if (it == m_NativeSurfacesByWindow.End() || it->second.IsNull())
			return;

		FocusSurface(it->second);
	}

	void FApplicationInstance::OnWindowMouseFocusChanged(FWindowHandle window, bool gotFocus)
	{
		if (!gotFocus)
			return;

		auto it = m_NativeSurfacesByWindow.Find(window);
		if (it == m_NativeSurfacesByWindow.End() || it->second.IsNull())
			return;

		FocusSurface(it->second);
	}

	void FApplicationInstance::OnWindowDisplayChanged(FWindowHandle window, FDisplayId displayId)
	{
		f32 scale = GetDpiScaleForWindow(window);
		FUSION_LOG_INFO("Application", "OnWindowDisplayChanged called: {} ; {}", displayId.Get(), scale);
	}

	void FApplicationInstance::NotifyWindowResize(FWindowHandle window)
	{
		auto it = m_NativeSurfacesByWindow.Find(window);
		if (it != m_NativeSurfacesByWindow.End() && it->second != nullptr)
		{
			it->second->OnWindowResized();
		}
	}
} // namespace Fusion

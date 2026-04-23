#include "Fusion/Widgets.h"

namespace Fusion
{
	FSlottedWidget::FSlottedWidget()
	{
		m_ClipShape = FRectangle();
	}

	void FSlottedWidget::OnBeforeDestroy()
	{
		Super::OnBeforeDestroy();

	}

	void FSlottedWidget::Construct()
	{
		Super::Construct();

		m_Slots.Resize(GetSlotCount());
	}

	void FSlottedWidget::SetParentSurfaceRecursive(Ref<FSurface> surface)
	{
		Super::SetParentSurfaceRecursive(surface);

		for (auto slot : m_Slots)
		{
			if (slot.IsValid())
			{
				slot->SetParentSurfaceRecursive(surface);
			}
		}
	}

	bool FSlottedWidget::SetSlotWidget(u32 slot, Ref<FWidget> widget)
	{
		if (slot >= GetSlotCount() || !widget || !IsValidSlotWidget(slot, widget))
			return false;

		if (m_Slots[slot].IsValid())
		{
			DetachChildWidget(m_Slots[slot]);
		}

		m_Slots[slot] = widget;

		if (m_Slots[slot])
		{
			AttachChildWidget(m_Slots[slot]);
		}

		return true;
	}

	int FSlottedWidget::GetChildCount()
	{
		int count = 0;

		for (auto slot : m_Slots)
		{
			if (slot) count++;
		}

		return count;
	}

	Ref<FWidget> FSlottedWidget::GetChildAt(u32 index)
	{
		u32 current = 0;

		for (auto slot : m_Slots)
		{
			if (slot)
			{
				if (current == index) 
					return slot;
				current++;
			}
		}

		return nullptr;
	}

	void FSlottedWidget::DetachChild(Ref<FWidget> child)
	{
		Super::DetachChild(child);

		for (int i = 0; i < m_Slots.Size(); i++)
		{
			if (m_Slots[i] == child)
			{
				DetachChildWidget(child);
				m_Slots[i] = nullptr;
				return;
			}
		}
	}

} // namespace Fusion

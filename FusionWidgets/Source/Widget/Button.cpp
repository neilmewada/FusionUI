#include "Fusion/Widgets.h"

namespace Fusion
{
	FButton::FButton()
	{
		
	}

	void FButton::OnMouseEnter(FMouseEvent& event)
	{
		Super::OnMouseEnter(event);

		if (!Disabled())
		{
			SetStyleStateFlag(EStyleState::Hovered, true);
		}
	}

	void FButton::OnMouseLeave(FMouseEvent& event)
	{
		Super::OnMouseLeave(event);

		SetStyleStateFlag(EStyleState::Hovered, false);
	}

	FEventReply FButton::OnMouseButtonDown(FMouseEvent& event)
	{
		if (Disabled())
			return FEventReply::Unhandled();

		if (event.IsLeftButton() || event.IsRightButton())
		{
			SetStyleStateFlag(EStyleState::Pressed, true);
		}
		return FEventReply::Handled();
	}

	FEventReply FButton::OnMouseButtonUp(FMouseEvent& event)
	{
		if (Disabled())
		{
			SetStyleStateFlag(EStyleState::Pressed | EStyleState::Hovered, false);

			return FEventReply::Unhandled();
		}

		if (event.IsLeftButton() || event.IsRightButton())
		{
			SetStyleStateFlag(EStyleState::Pressed, false);

			if (event.bIsInside)
			{
				m_OnClick.Broadcast(this);
			}
		}
		return FEventReply::Handled();
	}

} // namespace Fusion

#include "Fusion/Widgets.h"

namespace Fusion
{
	FButton::FButton()
	{
		
	}

	void FButton::OnMouseEnter(FMouseEvent& event)
	{
		Super::OnMouseEnter(event);

		SetStyleStateFlag(EStyleState::Hovered, true);
	}

	void FButton::OnMouseLeave(FMouseEvent& event)
	{
		Super::OnMouseLeave(event);

		SetStyleStateFlag(EStyleState::Hovered, false);
	}

	FEventReply FButton::OnMouseButtonDown(FMouseEvent& event)
	{
		if (event.IsLeftButton())
		{
			
		}
		return FEventReply::Handled();
	}

	FEventReply FButton::OnMouseButtonUp(FMouseEvent& event)
	{
		if (event.IsLeftButton())
		{
			if (event.bIsInside)
			{
				m_OnClick.Broadcast(this);
			}
		}
		return FEventReply::Handled();
	}

} // namespace Fusion

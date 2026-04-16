#include "Fusion/Widgets.h"

namespace Fusion
{
	FButton::FButton()
	{
		SetWidgetFlag(EWidgetFlags::Focusable, true);
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

		return FEventReply::Handled().FocusSelf();
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

	FEventReply FButton::OnKeyDown(FKeyEvent& event)
	{
		if (Disabled())
			return FEventReply::Unhandled();

		if (event.Key == EKeyCode::Space || event.Key == EKeyCode::Return)
		{
			SetStyleStateFlag(EStyleState::Pressed | EStyleState::Hovered, true);
			return FEventReply::Handled();
		}

		if (event.Key == EKeyCode::Tab)
		{
			bool shift = FEnumHasFlag(event.Modifiers, EKeyModifier::Shift);
			return shift ? FEventReply::Handled().FocusPrev() : FEventReply::Handled().FocusNext();
		}

		return FEventReply::Unhandled();
	}

	FEventReply FButton::OnKeyUp(FKeyEvent& event)
	{
		if (Disabled())
			return FEventReply::Unhandled();

		if (event.Key == EKeyCode::Space || event.Key == EKeyCode::Return)
		{
			SetStyleStateFlag(EStyleState::Pressed | EStyleState::Hovered, false);
			m_OnClick.Broadcast(this);
			return FEventReply::Handled();
		}

		return FEventReply::Unhandled();
	}

	void FButton::OnFocusChanged(FFocusEvent& event)
	{
		SetStyleStateFlag(EStyleState::Focused, event.GotFocus());
	}

} // namespace Fusion

#include "Fusion/Widgets.h"

namespace Fusion
{
	FTextButton::FTextButton()
	{
		
	}

	void FTextButton::Construct()
	{
		Super::Construct();

		Child(
			FAssignNew(FLabel, m_Label)
			.InheritParentStyleState(true)
			.SubStyle("Label")
			.Text("Button")
			.HAlign(EHAlign::Center)
			.VAlign(EVAlign::Center)
		);
	}

} // namespace Fusion

#include "Fusion/Widgets.h"

namespace Fusion
{
	FTheme::FTheme(Ref<FTheme> parent) : m_Parent(parent)
	{

	}

	void FTheme::SetParent(Ref<FTheme> parent)
	{
		if (parent != this)
		{
			m_Parent = parent;
		}
	}

	Ref<FStyle> FTheme::FindStyle(const FName& name) const
	{
		auto it = m_Styles.Find(name);
		if (it != m_Styles.End() && it->second)
		{
			return it->second;
		}
		if (Ref<FTheme> parent = m_Parent.Lock())
		{
			return parent->FindStyle(name);
		}
		return nullptr;
	}

	FStyle& FTheme::Style(const FName& name)
	{
		Ref<FStyle> style;

		auto it = m_Styles.Find(name);
		if (it != m_Styles.End() && it->second)
		{
			style = it->second;
		}
		else
		{
			style = NewObject<FStyle>(this);
			style->m_StyleSheet = Ref(this);

			m_Styles[name] = style;
		}

		return *style;
	}
} // namespace Fusion

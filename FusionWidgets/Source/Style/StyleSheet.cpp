#include "Fusion/Widgets.h"

namespace Fusion
{
	FStyleSheet::FStyleSheet(Ref<FStyleSheet> parent) : m_Parent(parent)
	{
		
	}

	void FStyleSheet::SetParent(Ref<FStyleSheet> parent)
	{
		if (parent != this)
		{
			m_Parent = parent;
		}
	}

	Ref<FStyle> FStyleSheet::FindStyle(const FName& name) const
	{
		auto it = m_Styles.Find(name);
		if (it != m_Styles.End() && it->second)
		{
			return it->second;
		}
		if (Ref<FStyleSheet> parent = m_Parent.Lock())
		{
			return parent->FindStyle(name);
		}
		return nullptr;
	}

	FStyle& FStyleSheet::Style(const FName& name)
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

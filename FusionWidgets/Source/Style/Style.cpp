#include "Fusion/Widgets.h"

namespace Fusion
{

	FStyle& FStyle::Brush(const FName& propertyName, const FBrush& value, EStyleState state)
	{
		m_BrushValues[propertyName].Add(value, state);
		return *this;
	}

	FStyle& FStyle::Pen(const FName& propertyName, const FPen& value, EStyleState state)
	{
		m_PenValues[propertyName].Add(value, state);
		return *this;
	}

	FStyle& FStyle::Color(const FName& propertyName, const FColor& value, EStyleState state)
	{
		m_ColorValues[propertyName].Add(value, state);
		return *this;
	}

	FStyle& FStyle::Shape(const FName& propertyName, const FShape& value, EStyleState state)
	{
		m_ShapeValues[propertyName].Add(value, state);
		return *this;
	}

	FStyle& FStyle::Float(const FName& propertyName, f32 value, EStyleState state)
	{
		m_FloatValues[propertyName].Add(value, state);
		return *this;
	}

	FStyle& FStyle::Vec2(const FName& propertyName, const FVec2& value, EStyleState state)
	{
		m_Vec2Values[propertyName].Add(value, state);
		return *this;
	}

	FStyle& FStyle::Vec4(const FName& propertyName, const FVec4& value, EStyleState state)
	{
		m_Vec4Values[propertyName].Add(value, state);
		return *this;
	}

} // namespace Fusion

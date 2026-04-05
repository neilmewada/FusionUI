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

	bool FStyle::TryGet(const FName& propertyName, FBrush& outBrush, EStyleState state)
	{
		auto it = m_BrushValues.Find(propertyName);
		if (it == m_BrushValues.End())
			return false;
		outBrush = it->second.Resolve(state);
		return true;
	}

	bool FStyle::TryGet(const FName& propertyName, FPen& outPen, EStyleState state)
	{
		auto it = m_PenValues.Find(propertyName);
		if (it == m_PenValues.End())
			return false;
		outPen = it->second.Resolve(state);
		return true;
	}

	bool FStyle::TryGet(const FName& propertyName, FColor& outColor, EStyleState state)
	{
		auto it = m_ColorValues.Find(propertyName);
		if (it == m_ColorValues.End())
			return false;
		outColor = it->second.Resolve(state);
		return true;
	}

	bool FStyle::TryGet(const FName& propertyName, FShape& outShape, EStyleState state)
	{
		auto it = m_ShapeValues.Find(propertyName);
		if (it == m_ShapeValues.End())
			return false;
		outShape = it->second.Resolve(state);
		return true;
	}

	bool FStyle::TryGet(const FName& propertyName, f32& outFloat, EStyleState state)
	{
		auto it = m_FloatValues.Find(propertyName);
		if (it == m_FloatValues.End())
			return false;
		outFloat = it->second.Resolve(state);
		return true;
	}

	bool FStyle::TryGet(const FName& propertyName, FVec2& outVec2, EStyleState state)
	{
		auto it = m_Vec2Values.Find(propertyName);
		if (it == m_Vec2Values.End())
			return false;
		outVec2 = it->second.Resolve(state);
		return true;
	}

	bool FStyle::TryGet(const FName& propertyName, FVec4& outVec4, EStyleState state)
	{
		auto it = m_Vec4Values.Find(propertyName);
		if (it == m_Vec4Values.End())
			return false;
		outVec4 = it->second.Resolve(state);
		return true;
	}

} // namespace Fusion

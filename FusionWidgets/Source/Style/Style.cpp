#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

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

	FStyle& FStyle::Font(const FName& propertyName, const FFont& value, EStyleState state)
	{
		m_FontValues[propertyName].Add(value, state);
		return *this;
	}

	FStyle& FStyle::Transform(const FName& propertyName, const FAffineTransform& value, EStyleState state)
	{
		m_TransformValues[propertyName].Add(value, state);
		return *this;
	}

	FStyle& FStyle::Transition(const FName& propertyName, const FTransition& value)
	{
		m_PropertyTransitions[propertyName] = value;
		return *this;
	}

	FStyle& FStyle::CopyFrom(const FStyle& other)
	{
		for (auto& [name, val] : other.m_BrushValues)  m_BrushValues[name] = val;
		for (auto& [name, val] : other.m_PenValues)    m_PenValues[name]   = val;
		for (auto& [name, val] : other.m_ColorValues)  m_ColorValues[name] = val;
		for (auto& [name, val] : other.m_ShapeValues)  m_ShapeValues[name] = val;
		for (auto& [name, val] : other.m_FloatValues)  m_FloatValues[name] = val;
		for (auto& [name, val] : other.m_Vec2Values)   m_Vec2Values[name]  = val;
		for (auto& [name, val] : other.m_Vec4Values)   m_Vec4Values[name]  = val;
		for (auto& [name, val] : other.m_TransformValues)   m_TransformValues[name] = val;
		for (auto& [name, val] : other.m_PropertyTransitions)   m_PropertyTransitions[name]  = val;
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

	bool FStyle::TryGet(const FName& propertyName, FFont& outFont, EStyleState state)
	{
		auto it = m_FontValues.Find(propertyName);
		if (it == m_FontValues.End())
			return false;
		outFont = it->second.Resolve(state);
		return true;
	}

	bool FStyle::TryGet(const FName& propertyName, FAffineTransform& outTransform, EStyleState state)
	{
		auto it = m_TransformValues.Find(propertyName);
		if (it == m_TransformValues.End())
			return false;
		outTransform = it->second.Resolve(state);
		return true;
	}

	bool FStyle::TryGetTransition(const FName& propertyName, FTransition& outTransition)
	{
		auto it = m_PropertyTransitions.Find(propertyName);
		if (it == m_PropertyTransitions.End())
			return false;
		outTransition = it->second;
		return true;
	}

} // namespace Fusion

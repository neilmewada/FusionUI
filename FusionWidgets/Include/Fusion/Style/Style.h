#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FTheme;
    class FWidget;

    template<typename T>
    struct FStyleValue
    {
    private:
	    
        T m_Default = T();
        TArray<FPair<EStyleState, T>, 1> m_Overrides;

    public:

        void Add(const T& value, EStyleState state)
        {
	        for (int i = 0; i < m_Overrides.Size(); i++)
	        {
		        if (m_Overrides[i].first == state)
		        {
                    m_Overrides.RemoveAt(i);
                    break;
		        }
	        }

            if (state == EStyleState::Default)
            {
                m_Default = value;
            }
            else
            {
                m_Overrides.Add({ state, value });
            }
        }

        const T& Resolve(EStyleState activeStates) const
        {
            ZoneScoped;

            if (activeStates == EStyleState::Default)
                return m_Default;

            const T* best = &m_Default;
            int bestBits = 0; // m_Default is the floor; overrides need ≥1 bit to beat it

            for (const auto& [mask, value] : m_Overrides)
            {
                if (mask == EStyleState::Default)
                    continue; // m_Default handles the zero-mask case

                if (!FEnumHasAllFlags(activeStates, mask))
                    continue; // mask must be fully satisfied by active states

                int bits = std::popcount((u32)mask);
                if (bits >= bestBits) // >= : later entry wins on tie (CSS source-order rule)
                {
                    best = &value;
                    bestBits = bits;
                }
            }

            return *best;
        }
    };

    class FUSIONWIDGETS_API FStyle : public FObject
    {
        FUSION_CLASS(FStyle, FObject)
    public:

        FStyle& Brush(const FName& propertyName, const FBrush& value, EStyleState state = EStyleState::Default);
        FStyle& Pen(const FName& propertyName, const FPen& value, EStyleState state = EStyleState::Default);
        FStyle& Color(const FName& propertyName, const FColor& value, EStyleState state = EStyleState::Default);
        FStyle& Shape(const FName& propertyName, const FShape& value, EStyleState state = EStyleState::Default);
        FStyle& Float(const FName& propertyName, f32 value, EStyleState state = EStyleState::Default);
        FStyle& Vec2(const FName& propertyName, const FVec2& value, EStyleState state = EStyleState::Default);
        FStyle& Vec4(const FName& propertyName, const FVec4& value, EStyleState state = EStyleState::Default);
        FStyle& Font(const FName& propertyName, const FFont& value, EStyleState state = EStyleState::Default);
        FStyle& Transform(const FName& propertyName, const FAffineTransform& value, EStyleState state = EStyleState::Default);

        FStyle& Transition(const FName& propertyName, const FTransition& value);

        template<typename T>
        FStyle& Set(const FName& propertyName, const T& value, EStyleState state = EStyleState::Default)
        {
            if constexpr (std::is_same_v<T, FBrush>)       return Brush(propertyName, value, state);
            else if constexpr (std::is_same_v<T, FPen>)    return Pen(propertyName, value, state);
            else if constexpr (std::is_same_v<T, FColor>)  return Color(propertyName, value, state);
            else if constexpr (std::is_same_v<T, FShape>)  return Shape(propertyName, value, state);
            else if constexpr (std::is_same_v<T, f32>)     return Float(propertyName, value, state);
            else if constexpr (std::is_same_v<T, FVec2>)   return Vec2(propertyName, value, state);
            else if constexpr (std::is_same_v<T, FVec4>)   return Vec4(propertyName, value, state);
            else if constexpr (std::is_same_v<T, FFont>)   return Font(propertyName, value, state);
            else if constexpr (std::is_same_v<T, FAffineTransform>)   return Transform(propertyName, value, state);
            else static_assert(sizeof(T) == 0, "FStyle::Set — unsupported property type");

            return *this;
        }

        FStyle& CopyFrom(const FStyle& other);

        bool TryGet(const FName& propertyName, FBrush& outBrush, EStyleState state);
        bool TryGet(const FName& propertyName, FPen& outPen, EStyleState state);
        bool TryGet(const FName& propertyName, FColor& outColor, EStyleState state);
        bool TryGet(const FName& propertyName, FShape& outShape, EStyleState state);
        bool TryGet(const FName& propertyName, f32& outFloat, EStyleState state);
        bool TryGet(const FName& propertyName, FVec2& outVec2, EStyleState state);
        bool TryGet(const FName& propertyName, FVec4& outVec4, EStyleState state);
        bool TryGet(const FName& propertyName, FFont& outFont, EStyleState state);
        bool TryGet(const FName& propertyName, FAffineTransform& outTransform, EStyleState state);

        bool TryGetTransition(const FName& propertyName, FTransition& outTransition);

    private:

        WeakRef<FTheme> m_Theme;

        FHashMap<FName, FStyleValue<FBrush>> m_BrushValues;
        FHashMap<FName, FStyleValue<FPen>>   m_PenValues;
        FHashMap<FName, FStyleValue<FColor>> m_ColorValues;
        FHashMap<FName, FStyleValue<FShape>> m_ShapeValues;
        FHashMap<FName, FStyleValue<f32>>    m_FloatValues;
        FHashMap<FName, FStyleValue<FVec2>>  m_Vec2Values;
        FHashMap<FName, FStyleValue<FVec4>>  m_Vec4Values;
        FHashMap<FName, FStyleValue<FFont>>  m_FontValues;
        FHashMap<FName, FStyleValue<FAffineTransform>>  m_TransformValues;
        FHashMap<FName, FTransition>         m_PropertyTransitions;

        friend class FTheme;
    };
    
} // namespace Fusion

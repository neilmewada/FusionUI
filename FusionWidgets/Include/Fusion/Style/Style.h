#pragma once

namespace Fusion
{
    class FStyleSheet;
    class FWidget;

    template<typename T>
    struct FStyleValue
    {
    private:
	    
        T m_Default = T();
        FArray<FPair<EStyleState, T>, 1> m_Overrides;

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

        bool TryGet(const FName& propertyName, FBrush& outBrush, EStyleState state);
        bool TryGet(const FName& propertyName, FPen& outPen, EStyleState state);
        bool TryGet(const FName& propertyName, FColor& outColor, EStyleState state);
        bool TryGet(const FName& propertyName, FShape& outShape, EStyleState state);
        bool TryGet(const FName& propertyName, f32& outFloat, EStyleState state);
        bool TryGet(const FName& propertyName, FVec2& outVec2, EStyleState state);
        bool TryGet(const FName& propertyName, FVec4& outVec4, EStyleState state);

    private:

        WeakRef<FStyleSheet> m_StyleSheet;

        FHashMap<FName, FStyleValue<FBrush>> m_BrushValues;
        FHashMap<FName, FStyleValue<FPen>>   m_PenValues;
        FHashMap<FName, FStyleValue<FColor>> m_ColorValues;
        FHashMap<FName, FStyleValue<FShape>> m_ShapeValues;
        FHashMap<FName, FStyleValue<f32>>    m_FloatValues;
        FHashMap<FName, FStyleValue<FVec2>>  m_Vec2Values;
        FHashMap<FName, FStyleValue<FVec4>>  m_Vec4Values;

        friend class FStyleSheet;
    };
    
} // namespace Fusion

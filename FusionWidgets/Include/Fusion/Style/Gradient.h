#pragma once

namespace Fusion
{

    enum class EGradientType : u8
    {
	    Linear,
    	Radial,
    	Conical
    };
    FUSION_ENUM_CLASS(EGradientType);

	enum class EGradientExtend : u8
	{
		Clamp,
		Repeat,
		Reflect
	};
    FUSION_ENUM_CLASS(EGradientExtend);

    struct FGradientStop
    {
        FColor Color;
        float Position = 0.0f; // 0-1
    };

    struct FUSIONWIDGETS_API FGradient
    {
    public:

        // - Factories -

        static FGradient Linear(f32 angle = 0.0f, f32 startPoint = 0.0f, f32 endPoint = 1.0f);
        static FGradient Radial(FVec2 center, f32 radius);
        static FGradient Conical(FVec2 center, f32 angle);

        // - Builder -

        FGradient& AddStop(FColor color, f32 position);
        FGradient& SetExtend(EGradientExtend extend);

        // - Getters -

        bool IsValid()   const { return m_Stops.Size() >= 2; }
        bool IsLinear()  const { return m_Type == EGradientType::Linear; }
        bool IsRadial()  const { return m_Type == EGradientType::Radial; }
        bool IsConical() const { return m_Type == EGradientType::Conical; }

        EGradientType   GetType()   const { return m_Type; }
        EGradientExtend GetExtend() const { return m_Extend; }
        const auto&     GetStops()  const { return m_Stops; }
        auto&           GetStops()        { return m_Stops; }

        f32   GetAngle()      const { return m_Angle; }       // Linear + Conical
        f32   GetStartPoint() const { return m_StartPoint; }  // Linear
        f32   GetEndPoint()   const { return m_EndPoint; }    // Linear
        FVec2 GetCenter()     const { return m_Center; }      // Radial + Conical
        f32   GetRadius()     const { return m_Radius; }      // Radial

        bool operator==(const FGradient& rhs) const;
        bool operator!=(const FGradient& rhs) const { return !(*this == rhs); }

        // - Setters -

        void SetType(EGradientType type) { m_Type = type; }
        void SetAngle(f32 angle)         { m_Angle = angle; }

    private:

        FArray<FGradientStop, 2> m_Stops;

        f32   m_Angle      = 0.0f;
        f32   m_StartPoint = 0.0f;
        f32   m_EndPoint   = 1.0f;
        FVec2 m_Center     = FVec2(0.5f, 0.5f);
        f32   m_Radius     = 0.5f;

        EGradientType         m_Type = EGradientType::Linear;
        EGradientExtend       m_Extend = EGradientExtend::Clamp;
    };

} // namespace Fusion

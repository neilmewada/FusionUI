#pragma once

namespace Fusion
{

    enum class EGradientType
    {
	    Linear, 
    	Radial, 
    	Conical
    };
    FUSION_ENUM_CLASS(EGradientType);
    
	enum class EGradientExtend
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

        FGradient();


    private:
        EGradientType m_Type = EGradientType::Linear;
        EGradientExtend m_Extend = EGradientExtend::Clamp;
        FArray<FGradientStop> m_Stops;

        union
        {
            struct { FVec2 Start, End; } Linear;
            struct { FVec2 Center; float Radius; } Radial;
            struct { FVec2 Center; float Angle; } Conical;
        } m_Params;
    };
    
} // namespace Fusion

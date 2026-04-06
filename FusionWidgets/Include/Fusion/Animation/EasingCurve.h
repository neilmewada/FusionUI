#pragma once

namespace Fusion
{

    enum class EEasingType
    {
        Linear,
        EaseInQuad, EaseOutQuad, EaseInOutQuad,
        EaseInCubic, EaseOutCubic, EaseInOutCubic,
        EaseInQuart, EaseOutQuart, EaseInOutQuart,
        EaseInSine, EaseOutSine, EaseInOutSine,
        EaseInExpo, EaseOutExpo, EaseInOutExpo,
        EaseInCirc, EaseOutCirc, EaseInOutCirc,
        EaseInBack, EaseOutBack, EaseInOutBack,
        EaseInBounce, EaseOutBounce, EaseInOutBounce,
        EaseInElastic, EaseOutElastic, EaseInOutElastic,
        CubicBezier,   // Uses controlPoint1 / controlPoint2
    };
    FUSION_ENUM_CLASS(EEasingType);

    struct FUSIONWIDGETS_API FEasingCurve final
    {
    public:

        FEasingCurve() = default;

        FEasingCurve(EEasingType type, FVec2 controlPoint1 = {}, FVec2 controlPoint2 = {})
            : Type(type)
            , ControlPoint1(controlPoint1)
            , ControlPoint2(controlPoint2)
        {
        }

        f32 Evaluate(f32 t) const;

        EEasingType Type = EEasingType::Linear;

        FVec2 ControlPoint1;

        FVec2 ControlPoint2;

    };
    
} // namespace Fusion

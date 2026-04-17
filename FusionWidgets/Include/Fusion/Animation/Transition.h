#pragma once

namespace Fusion
{
    enum class ETransitionType : u8
    {
        Tween = 0,
        Spring
    };

    struct FUSIONWIDGETS_API FTransition
    {
        static FTransition MakeTween(float duration, FEasingCurve easing = EEasingType::EaseInOutCubic, float delay = 0.0f)
        {
            return {
                .Type = ETransitionType::Tween,
                .Tween = {
                    .Easing = easing,
                    .Duration = duration,
                    .Delay = delay,
                }
            };
        }

        static FTransition MakeSpring(float stiffness = 200.0f, float damping = 20.0f, float delay = 0.0f)
        {
            return {
                .Type = ETransitionType::Spring,
                .Spring = {
                    .Stiffness = stiffness,
                    .Damping = damping,
                    .Delay = delay,
                }
            };
        }

        struct FTweenTransition
        {
            FEasingCurve Easing = EEasingType::EaseInOutCubic;
            float Duration = 0.3f;
            float Delay = 0.0f;
        };

        struct FSpringTransition
        {
            float Stiffness = 200.0f;
            float Damping = 20.0f;
            float Delay = 0.0f;
        };

        ETransitionType Type = ETransitionType::Tween;
        union
        {
            FTweenTransition Tween;
            FSpringTransition Spring;
        };
    };

}
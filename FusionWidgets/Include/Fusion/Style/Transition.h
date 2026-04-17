#pragma once

namespace Fusion
{
    enum class ETransitionType : u8
    {
        None = 0,
        Tween,
        Spring
    };

    struct FUSIONWIDGETS_API FTransition final
    {
        struct FTweenTransition
        {
            FEasingCurve Easing   = EEasingType::EaseInOutCubic;
            float        Duration = 0.3f;
            float        Delay    = 0.0f;
        };

        struct FSpringTransition
        {
            float Stiffness = 200.0f;
            float Damping   = 20.0f;
            float Delay     = 0.0f;
        };

        ETransitionType Type = ETransitionType::None;
        union
        {
            FTweenTransition  Tween;
            FSpringTransition Spring;
        };

        FTransition() : Type(ETransitionType::None) {}

        FTransition(const FTransition& other) : Type(other.Type)
        {
            CopyFrom(other);
        }

        FTransition(FTransition&& other) noexcept : Type(other.Type)
        {
            MoveFrom(std::move(other));
        }

        FTransition& operator=(const FTransition& other)
        {
            if (this != &other)
            {
                Destroy();
                Type = other.Type;
                CopyFrom(other);
            }
            return *this;
        }

        FTransition& operator=(FTransition&& other) noexcept
        {
            if (this != &other)
            {
                Destroy();
                Type = other.Type;
                MoveFrom(std::move(other));
            }
            return *this;
        }

        ~FTransition()
        {
            Destroy();
        }

        static FTransition MakeTween(float duration, FEasingCurve easing = EEasingType::EaseInOutCubic, float delay = 0.0f)
        {
            FTransition t;
            t.Type = ETransitionType::Tween;
            new (&t.Tween) FTweenTransition{ easing, duration, delay };
            return t;
        }

        static FTransition MakeSpring(float stiffness = 200.0f, float damping = 20.0f, float delay = 0.0f)
        {
            FTransition t;
            t.Type = ETransitionType::Spring;
            new (&t.Spring) FSpringTransition{ stiffness, damping, delay };
            return t;
        }

    private:

        void CopyFrom(const FTransition& other)
        {
            if (Type == ETransitionType::Tween)
                new (&Tween) FTweenTransition(other.Tween);
            else if (Type == ETransitionType::Spring)
                new (&Spring) FSpringTransition(other.Spring);
        }

        void MoveFrom(FTransition&& other)
        {
            if (Type == ETransitionType::Tween)
                new (&Tween) FTweenTransition(std::move(other.Tween));
            else if (Type == ETransitionType::Spring)
                new (&Spring) FSpringTransition(std::move(other.Spring));
        }

        void Destroy()
        {
            if (Type == ETransitionType::Tween)
                Tween.~FTweenTransition();
            else if (Type == ETransitionType::Spring)
                Spring.~FSpringTransition();
        }
    };

} // namespace Fusion

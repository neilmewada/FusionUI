#pragma once

namespace Fusion
{
    // Internal interface
    class IFInterpolator
    {
    public:

        virtual void Apply(f32 t) = 0;

        virtual ~IFInterpolator() = default;
    };

    template<typename T>
    class FInterpolator : public IFInterpolator
    {
    public:

        FInterpolator(T from, T to, FEasingCurve easing, std::function<void(const T&)> setter)
            : from(MoveTemp(from))
            , to(MoveTemp(to))
            , easing(easing)
            , setter(MoveTemp(setter))
        {
        }

        void Apply(f32 t) override
        {
            setter(FAnimatable<T>::Lerp(from, to, easing.Evaluate(t)));
        }

    private:
        T from{};
        T to{};
        FEasingCurve easing{};
        std::function<void(const T&)> setter{};
    };

    class FUSIONWIDGETS_API FTweenAnimation : public FAnimation
    {
        FUSION_CLASS(FTweenAnimation, FAnimation)
    protected:

        FTweenAnimation(FName name);

    public:

        template<typename T>
        void SetInterpolator(T from, T to, FEasingCurve easing, std::function<void(const T&)> setter)
        {
            m_Interpolator.Reset(new FInterpolator<T>(
                MoveTemp(from), MoveTemp(to), easing, MoveTemp(setter)));
        }

        void Apply(f32 normalizedTime) override;

    private:

        TUniquePtr<IFInterpolator> m_Interpolator;
    };
    
} // namespace Fusion

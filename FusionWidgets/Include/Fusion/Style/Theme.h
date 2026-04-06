#pragma once

namespace Fusion
{

    class FUSIONWIDGETS_API FTheme : public FObject
    {
        FUSION_CLASS(FTheme, FObject)
    public:

        FTheme(Ref<FTheme> parent = nullptr);

        // - Public API -

        Ref<FTheme> GetParent() const { return m_Parent.Lock(); }

        void SetParent(Ref<FTheme> parent);

        Ref<FStyle> FindStyle(const FName& name) const;

        // - Builder API -

        FStyle& Style(const FName& name);

        template<typename TCallable>
        void Merge(TCallable&& callable)
        {
            callable(this);
        }

    private:

        WeakRef<FTheme> m_Parent;

        FHashMap<FName, Ref<FStyle>> m_Styles;
    };

    using FStyleSheet = void(*)(FTheme*);

} // namespace Fusion

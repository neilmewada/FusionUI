#pragma once

namespace Fusion
{
    class FTheme;

    using FStyleSheet = void(*)(FTheme*);

    class FUSIONWIDGETS_API FTheme : public FObject
    {
        FUSION_CLASS(FTheme, FObject)
    public:

        FTheme(Ref<FTheme> parent = nullptr);

        // - Public API -

        Ref<FTheme> GetParent() const { return m_Parent.Lock(); }

        void SetParent(Ref<FTheme> parent);

        Ref<FStyle> FindStyle(const FName& name) const;

        void ClearAll();

        // - Builder API -

        FStyle& Style(const FName& name);

        void MergeStyleSheet(FStyleSheet&& styleSheet)
        {
            styleSheet(this);
        }

    private:

        WeakRef<FTheme> m_Parent;

        THashMap<FName, Ref<FStyle>> m_Styles;
    };

} // namespace Fusion

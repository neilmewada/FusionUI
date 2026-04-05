#pragma once

namespace Fusion
{

    class FUSIONWIDGETS_API FStyleSheet : public FObject
    {
        FUSION_CLASS(FStyleSheet, FObject)
    public:

        FStyleSheet(Ref<FStyleSheet> parent = nullptr);

        // - Public API -

        Ref<FStyleSheet> GetParent() const { return m_Parent.Lock(); }

        void SetParent(Ref<FStyleSheet> parent);

        Ref<FStyle> FindStyle(const FName& name) const;
        
        // - Builder API -

        FStyle& Style(const FName& name);

    private:

        WeakRef<FStyleSheet> m_Parent;

        FHashMap<FName, Ref<FStyle>> m_Styles;
    };
    
} // namespace Fusion

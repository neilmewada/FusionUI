#pragma once

namespace Fusion
{
    class FItemView;
    FUSION_SIGNAL_TYPE(FItemSelectionChanged, const FModelIndexList& added, const FModelIndexList& removed);

    class FUSIONWIDGETS_API FItemSelectionModel : public FObject
    {
        FUSION_CLASS(FItemSelectionModel, FObject)
    protected:

        FItemSelectionModel();

    public:

        enum ESelectionFlags
        {
            Select_None = 0,
            Select_Clear = FUSION_BIT(0),
            Select_Current = FUSION_BIT(1),
            Select_Toggle = FUSION_BIT(2),
        };

        void ClearSelection();

        void Select(FModelIndex index, ESelectionFlags selectionFlags);

        bool IsSelected(FModelIndex index);

    private:

        FModelIndex m_CurrentIndex;
        FModelIndexList m_SelectedIndices;

    public:

        // - Fusion Signals -

        FUSION_SIGNAL(FItemSelectionChanged, OnSelectionChanged);

    };
    FUSION_ENUM_FLAGS(FItemSelectionModel::ESelectionFlags);

}
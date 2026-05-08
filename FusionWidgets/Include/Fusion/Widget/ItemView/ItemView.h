#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    class FUSIONWIDGETS_API FItemView : public FDecoratedBox
    {
        FUSION_WIDGET(FItemView, FDecoratedBox)
    protected:

        FItemView();

        void Construct() override;

    public:

        enum ESelectionBehavior : u8
        {
            SelectRows = 0,
        };

        enum ESelectionMode : u8
        {
            NoSelection = 0,
            SingleSelection,
            ExtendedSelection,
        };

        virtual bool IsExpandable() const { return false; }

        virtual bool IsExpanded(FModelIndex index) { return false; }

    protected:

        virtual void OnModelChanged();

        virtual void OnSelectionChanged(const FModelIndexList& added, const FModelIndexList& removed);

    private:

        Ref<FItemModel> m_Model;
        Ref<FItemSelectionModel> m_SelectionModel;

    public:

        // - Fusion Properties -

        FUSION_PROPERTY_GET(Ref<FItemModel>, Model)
        {
            return m_Model;
        }

        FUSION_PROPERTY_SET(Ref<FItemModel>, Model)
        {
            if (self.m_Model == value)
                return self;
            static_cast<FItemView&>(self).m_Model = value;
            static_cast<FItemView&>(self).OnModelChanged();
            return self;
        }

        FUSION_PROPERTY_GET(Ref<FItemSelectionModel>, SelectionModel)
        {
            return m_SelectionModel;
        }

        FUSION_STATE_PROPERTY(Ref<FItemViewDelegate>, ItemDelegate);

        FUSION_STYLE_PROPERTIES(
            (f32,  RowIconWidth,   Paint),
            (f32,  RowIconGap,     Paint),
            (f32,  RowChevronSize, Paint),
            (f32,  RowChevronGap,  Paint),
            (f32,  RowIndentWidth, Paint),
            (f32,  RowLeftPadding, Paint),
            (FPen, TreeLinePen,    Paint)
        );

        FUSION_STATE_PROPERTY(ESelectionMode, SelectionMode);
        FUSION_STATE_PROPERTY(ESelectionBehavior, SelectionBehavior);
    };

}
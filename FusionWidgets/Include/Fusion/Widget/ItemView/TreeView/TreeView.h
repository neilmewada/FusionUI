#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FTreeViewHeader;
    class FTreeViewContent;
    typedef FDelegate<f32(FModelIndex)> FRowHeightDelegate;

    class FUSIONWIDGETS_API FTreeView : public FItemView
    {
        FUSION_WIDGET(FTreeView, FItemView)
    protected:

        FTreeView();

        void Construct() override;

    public:

        bool IsExpandable() const override { return true; }

        Ref<FTreeViewHeader> GetHeader() const { return m_Header; }

        Ref<FTreeViewContent> GetContent() const { return m_Content; }

        bool IsExpanded(FModelIndex index) override;

    protected:

        void OnModelChanged() override;

        Ref<FVerticalStack> m_Container;
        Ref<FTreeViewHeader> m_Header;
        Ref<FScrollBox> m_ScrollBox;
        Ref<FTreeViewContent> m_Content;

    public:
        // - Fusion Properties -

        FUSION_STATE_PROPERTY(FDelegate<FTreeViewRow&()>, RowDelegate);

        FUSION_PROPERTY_GET(bool, CanResizeColumns)
        {
            return m_Header->CanResizeColumns();
        }
        FUSION_PROPERTY_SET(bool, CanResizeColumns)
        {
            static_cast<Self&>(self).m_Header->CanResizeColumns(value);
            return self;
        }

        FUSION_LAYOUT_PROPERTY(f32, RowHeight);
    };
    
} // namespace Fusion

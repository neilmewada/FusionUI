#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FTreeView;

    class FUSIONWIDGETS_API FTreeViewContent : public FWidget
    {
        FUSION_WIDGET(FTreeViewContent, FWidget)
    protected:

        FTreeViewContent();

        void Construct() override;

    public:

        Ref<FTreeView> GetFTreeView() const { return m_TreeView.Lock(); }

    protected:

        WeakRef<FTreeView> m_TreeView;

    public:

        FUSION_PROPERTY_SET(Ref<FTreeView>, TreeView)
        {
            self.m_TreeView = value;
            return self;
        }

    };
    
} // namespace Fusion

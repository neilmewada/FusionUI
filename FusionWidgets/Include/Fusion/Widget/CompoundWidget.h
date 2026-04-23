#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    class FUSIONWIDGETS_API FCompoundWidget : public FSlottedWidget
    {
        FUSION_CLASS(FCompoundWidget, FSlottedWidget)
    public:

        FCompoundWidget();

        // - Public API -

        Ref<FWidget> GetChild() const { return m_Child; }

        void SetInternalPadding(FMargin padding)
        {
            if (m_InternalPadding == padding)
                return;
	        m_InternalPadding = padding; 
        	MarkLayoutDirty();
        }

        // - Layout -

        FVec2 MeasureContent(FVec2 availableSize) override;

        void ArrangeContent(FVec2 finalSize) override;

        // - Paint -


    private:

        void SetWidgetFlagInternal(EWidgetFlags flag, bool set);

        FMargin m_InternalPadding;

    public: // - Fusion Properties - 

    	FUSION_SLOTS(
            (FWidget, Child)
        );

        FUSION_PROPERTY_SET(bool, ForcePaintBoundary)
        {
            if (self.TestWidgetFlags(EWidgetFlags::ForcePaintBoundary) == value)
                return self;

            self.SetWidgetFlagInternal(EWidgetFlags::ForcePaintBoundary, value);
            self.UpdateBoundaryFlags();
            return self;
        }
    };
    
} // namespace Fusion

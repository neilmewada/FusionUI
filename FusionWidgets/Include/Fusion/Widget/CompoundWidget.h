#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    class FUSIONWIDGETS_API FCompoundWidget : public FWidget
    {
        FUSION_CLASS(FCompoundWidget, FWidget)
    public:

        FCompoundWidget();

        // - Public API -

        void SetParentSurfaceRecursive(Ref<FSurface> surface) override;

        void DetachChild(Ref<FWidget> child) override;

        Ref<FWidget> GetChild() const { return m_Child; }

        int GetChildCount() override { return m_Child.IsValid() ? 1 : 0; }

        Ref<FWidget> GetChildAt(u32 index) override { return index == 0 ? m_Child : nullptr; }

        void SetChild(Ref<FWidget> child);

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

        Ref<FWidget> m_Child;
        FMargin m_InternalPadding;

    public: // - Fusion Properties - 

        FUSION_PROPERTY_SET(FWidget&, Child)
        {
            self.SetChild(&value);
            return self;
        }

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

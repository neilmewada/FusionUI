#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    
    class FUSIONWIDGETS_API FContainerWidget : public FWidget
    {
        FUSION_CLASS(FContainerWidget, FWidget)
    public:

        FContainerWidget();

        // - Public API -

        FShape GetClipShape() const override { return ClipContent() ? ClipShape() : EShapeType::None; }

        void SetParentSurfaceRecursive(Ref<FSurface> surface) override;

        void AddChildWidget(Ref<FWidget> childWidget);

        void AddChild(FWidget& child)
        {
            AddChildWidget(&child);
        }

        void RemoveChildWidget(Ref<FWidget> childWidget);

        void DetachChild(Ref<FWidget> child) override;

        void SetChildIndex(Ref<FWidget> childWidget, int index);

        int GetChildCount() override { return (u32)m_Children.Size(); }

        Ref<FWidget> GetChildAt(u32 index) override
        {
            if (index >= (u32)m_Children.Size())
            {
                return nullptr;
            }
            return m_Children[index];
        }

    private: // - Internal -

        void SetWidgetFlagInternal(EWidgetFlags flag, bool set);

        TArray<Ref<FWidget>> m_Children;

    public: // - Fusion Properties -

        FUSION_PROPERTY_SET(bool, ForcePaintBoundary)
        {
            if (self.TestWidgetFlags(EWidgetFlags::ForcePaintBoundary) == value)
                return self;

            self.SetWidgetFlagInternal(EWidgetFlags::ForcePaintBoundary, value);
            self.UpdateBoundaryFlags();
            return self;
        }

        FUSION_PROPERTY(FShape, ClipShape);
        FUSION_PROPERTY(bool, ClipContent);
    };

} // namespace Fusion

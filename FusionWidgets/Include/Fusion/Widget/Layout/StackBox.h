#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    enum class EStackDirection
    {
        Horizontal,
        Vertical
    };
    FUSION_ENUM_CLASS(EStackDirection);

    class FUSIONWIDGETS_API FStackBox : public FContainerWidget
    {
        FUSION_CLASS(FStackBox, FContainerWidget)
    public:

        FStackBox();

        // - Public API -

        void SetParentSurfaceRecursive(Ref<FSurface> surface) override;

        // - Layout -

        FVec2 MeasureContent(FVec2 availableSize) override;

        void ArrangeContent(FVec2 finalSize) override;

        template<typename... TArgs>
        struct TValidate_Children : TFFalseType
        {
        };

        template<typename T>
        struct TValidate_Children<T> : TFBoolConst<TFIsDerivedClass<FWidget, T>::Value or TFIsDerivedClass<FWidgetBuilder, T>::Value>
        {
        };

        template<typename TFirst, typename... TRest>
        struct TValidate_Children<TFirst, TRest...> : TFBoolConst<TValidate_Children<TFirst>::Value and TValidate_Children<TRest...>::Value>
        {
        };

        template<typename TSelf, typename... TArgs> requires TValidate_Children<TArgs...>::Value and (sizeof...(TArgs) > 0)
        TSelf& operator()(this TSelf& self, const TArgs&... childWidget)
        {
            using TupleType = std::tuple<const TArgs&...>;
            TupleType args = { childWidget... };

            constexpr_for<0, sizeof...(TArgs), 1>([&](auto i)
                {
                    using ArgTypeBase = std::tuple_element_t<i(), TupleType>;
                    using ArgType = std::remove_cvref_t<ArgTypeBase>;
                    if constexpr (TFIsDerivedClass<FWidget, ArgType>::Value)
                    {
                        self.AddChildWidget(const_cast<ArgType*>(&std::get<i()>(args)));
                    }
                    else if constexpr (TFIsDerivedClass<FWidgetBuilder, ArgType>::Value)
                    {
                        const_cast<ArgType*>(&std::get<i()>(args))->Build(&self);
                    }
                });

            return self;
        }

    public: // - Fusion Properties - 

        FUSION_LAYOUT_PROPERTY(EStackDirection, Direction);
        FUSION_LAYOUT_PROPERTY(f32, Spacing);
        FUSION_LAYOUT_PROPERTY(EHAlign, ContentHAlign);
        FUSION_LAYOUT_PROPERTY(EVAlign, ContentVAlign);

    };
    
    class FUSIONWIDGETS_API FVerticalStack : public FStackBox
    {
        FUSION_CLASS(FVerticalStack, FStackBox)
    public:

        FVerticalStack()
        {
            m_Direction = EStackDirection::Vertical;
        }

        void OnPropertyModified(const FName& propertyName) override;
        
    };

    class FUSIONWIDGETS_API FHorizontalStack : public FStackBox
    {
        FUSION_CLASS(FHorizontalStack, FStackBox)
    public:

        FHorizontalStack()
        {
            m_Direction = EStackDirection::Horizontal;
        }

        void OnPropertyModified(const FName& propertyName) override;

    };

} // namespace Fusion

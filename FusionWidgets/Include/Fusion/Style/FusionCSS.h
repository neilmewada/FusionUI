#pragma once

namespace Fusion::CSS
{

    struct FStyleContext
    {
        FStyle&     Style;
        EStyleState State = EStyleState::Default;

        FStyleContext(FStyle& style) : Style(style) {}
    };

    struct FStateScope
    {
        FStyleContext& m_Context;
        EStyleState    m_Prev;

        FStateScope(FStyleContext& context, EStyleState state)
            : m_Context(context), m_Prev(context.State)
        {
            m_Context.State = state;
        }

        ~FStateScope()
        {
            m_Context.State = m_Prev;
        }

        explicit operator bool() const { return true; }
    };

    template<typename T>
    struct FStyleProp
    {
        FStyleContext* m_Context;
        FName          m_Name;

        FStyleProp(FStyleContext* context, const FName& name)
            : m_Context(context), m_Name(name) {}

        FStyleProp& operator=(const T& value)
        {
            m_Context->Style.Set<T>(m_Name, value, m_Context->State);
            return *this;
        }
    };

} // namespace Fusion::CSS


#define __FUSION_CSS_MAKE_PROP(WidgetClass, PropName)                                  \
    Fusion::CSS::FStyleProp<                                                           \
        std::remove_cvref_t<decltype(std::declval<const WidgetClass&>().PropName())>  \
    >{&_fusion_css_ctx, #PropName},



/// @brief Opens a named style rule block, binding each listed property as a typed local variable.
///
/// Each property name in @p ... is brought into scope as an `FStyleProp<T>`, where `T` is
/// deduced from the corresponding const getter on @p WidgetClass. Assignments inside the block
/// target the Default state. Nest FUSION_ON inside to set state overrides.
///
/// @param WidgetClass  The widget type whose getters are used to deduce each property's type.
/// @param StyleName    The string key to register this rule under in the theme (e.g. "Button/Primary").
/// @param ...          One or more property names (e.g. Shape, Background, Border).
///
/// @example
/// FUSION_STYLE(FButton, "Button/Primary", Shape, Background, Border)
/// {
///     Shape      = FRoundedRectangle(5.0f);
///     Background = FColor(0.23f, 0.51f, 0.96f);
///     Border     = FColor(0.16f, 0.40f, 0.82f);
///
///     FUSION_ON(Hovered) { Background = FColor(0.38f, 0.65f, 0.98f); }
/// }
#define FUSION_STYLE(WidgetClass, StyleName, ...)                                      \
    if (Fusion::CSS::FStyleContext _fusion_css_ctx{theme->Style(StyleName)};     \
        true)                                                                          \
	if ([[maybe_unused]] auto Extends = [&](const FName& baseStyleName) -> void {      \
			if (Ref<FStyle> parent = theme->FindStyle(baseStyleName))                       \
				_fusion_css_ctx.Style.CopyFrom(*parent);                                \
		}; true)                                                                        \
    if ([[maybe_unused]] auto Transition = [&]<class T>(const Fusion::CSS::FStyleProp<T>& property, const ::Fusion::FTransition& transition)\
    {\
            _fusion_css_ctx.Style.Transition(property.m_Name, transition);\
        }; true)\
    if (auto [__VA_ARGS__] = std::tuple{                                               \
            FUSION_MACRO_EXPAND(                                                       \
                FUSION_FOR_EACH_CTX(__FUSION_CSS_MAKE_PROP, WidgetClass, __VA_ARGS__)) \
        }; true)


#define __FUSION_FOLD_STATES_1(a)                       EStyleState::a
#define __FUSION_FOLD_STATES_2(a, b)                    EStyleState::a | EStyleState::b
#define __FUSION_FOLD_STATES_3(a, b, c)                 EStyleState::a | EStyleState::b | EStyleState::c
#define __FUSION_FOLD_STATES_4(a, b, c, d)              EStyleState::a | EStyleState::b | EStyleState::c | EStyleState::d
#define __FUSION_FOLD_STATES_5(a, b, c, d, e)           EStyleState::a | EStyleState::b | EStyleState::c | EStyleState::d | EStyleState::e
#define __FUSION_FOLD_STATES_6(a, b, c, d, e, f)        EStyleState::a | EStyleState::b | EStyleState::c | EStyleState::d | EStyleState::e | EStyleState::f
#define __FUSION_FOLD_STATES_7(a, b, c, d, e, f, g)     EStyleState::a | EStyleState::b | EStyleState::c | EStyleState::d | EStyleState::e | EStyleState::f | EStyleState::g
#define __FUSION_FOLD_STATES_7(a, b, c, d, e, f, g, h)  EStyleState::a | EStyleState::b | EStyleState::c | EStyleState::d | EStyleState::e | EStyleState::f | EStyleState::g | EStyleState::h

// For internal use only!
#define __FUSION_FOLD_STATES(...) \
    FUSION_MACRO_EXPAND(FUSION_CONCATENATE(__FUSION_FOLD_STATES_, FUSION_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__))



/// @brief Opens a state-override block inside a FUSION_STYLE body.
///
/// @param ...  One or more bare EStyleState names without the `EStyleState::` prefix
///             (e.g. Hovered, Pressed). Multiple names are combined with | — all listed
///             states must be active simultaneously for the override to apply.
///
/// @example
/// FUSION_ON(Hovered)          { Background = FColor(0.38f, 0.65f, 0.98f); }
/// FUSION_ON(Pressed, Hovered) { Background = FColor(0.11f, 0.31f, 0.85f); }
#define FUSION_ON(...)                                                                 \
    if (Fusion::CSS::FStateScope _fusion_state_scope{                                 \
            _fusion_css_ctx,                                                           \
            __FUSION_FOLD_STATES(__VA_ARGS__)};                                          \
        true)

/// @brief Defines a stylesheet lambda compatible with FTheme::MergeStyleSheet().
///
/// @example
/// theme->MergeStyleSheet(FUSION_STYLE_SHEET
/// {
///     FUSION_STYLE(FButton, "Button/Primary", Shape, Background, Border) { ... }
/// });
#define FUSION_STYLE_SHEET [](FTheme* theme) -> void

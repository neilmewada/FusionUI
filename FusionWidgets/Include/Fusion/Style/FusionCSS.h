#pragma once

namespace Fusion::CSS
{
    // -------------------------------------------------------------------------
    // FStyleContext — stack-allocated, owns the active style ref + current state.
    // Shared by all FStyleProp instances in a FUSION_STYLE block via pointer.
    // -------------------------------------------------------------------------

    struct FStyleContext
    {
        FStyle&     Style;
        EStyleState State = EStyleState::Default;

        FStyleContext(FStyle& style) : Style(style) {}
    };

    // -------------------------------------------------------------------------
    // FStateScope — RAII, temporarily changes the state on the shared context.
    // Restored on destruction so nested FUSION_ON blocks compose correctly.
    // -------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// __FUSION_CSS_MAKE_PROP(WidgetClass, PropName)
//   Produces one FStyleProp<T> where T is deduced from the widget getter.
//   Uses std::remove_cvref_t so const& return types reduce to plain T.
//   Trailing comma is valid in a braced-init-list (C++11).
// -----------------------------------------------------------------------------

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
    if (Fusion::CSS::FStyleContext _fusion_css_ctx{styleSheet->Style(StyleName)};     \
        true)                                                                          \
	if (auto Extends = [&](const FName& name) -> void {                                \
			if (Ref<FStyle> parent = styleSheet->FindStyle(name))                       \
				_fusion_css_ctx.Style.CopyFrom(*parent);                                \
		}; true)                                                                        \
    if (auto [__VA_ARGS__] = std::tuple{                                               \
            FUSION_MACRO_EXPAND(                                                       \
                FUSION_FOR_EACH_CTX(__FUSION_CSS_MAKE_PROP, WidgetClass, __VA_ARGS__)) \
        }; true)



#define __FUSION_FOLD_STATES_1(a)                   EStyleState::a
#define __FUSION_FOLD_STATES_2(a, b)                EStyleState::a | EStyleState::b
#define __FUSION_FOLD_STATES_3(a, b, c)             EStyleState::a | EStyleState::b | EStyleState::c
#define __FUSION_FOLD_STATES_4(a, b, c, d)          EStyleState::a | EStyleState::b | EStyleState::c | EStyleState::d
#define __FUSION_FOLD_STATES_5(a, b, c, d, e)       EStyleState::a | EStyleState::b | EStyleState::c | EStyleState::d | EStyleState::e
#define __FUSION_FOLD_STATES_6(a, b, c, d, e, f)    EStyleState::a | EStyleState::b | EStyleState::c | EStyleState::d | EStyleState::e | EStyleState::f
#define __FUSION_FOLD_STATES_7(a, b, c, d, e, f, g) EStyleState::a | EStyleState::b | EStyleState::c | EStyleState::d | EStyleState::e | EStyleState::f | EStyleState::g

/// @brief Folds one or more bare state names into a combined `EStyleState` bitmask.
///
/// Each name is prefixed with `EStyleState::` and the results are combined with `|`.
/// Used internally by FUSION_ON, but available directly when a raw state mask is needed.
///
/// @param ...  One or more bare EStyleState names (e.g. Hovered, Pressed).
///
/// @example
/// EStyleState mask = FUSION_FOLD_STATES(Hovered, Pressed);
/// // expands to: EStyleState::Hovered | EStyleState::Pressed
#define FUSION_FOLD_STATES(...) \
    FUSION_MACRO_EXPAND(FUSION_CONCATENATE(__FUSION_FOLD_STATES_, FUSION_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__))



/// @brief Opens a state-override block inside a FUSION_STYLE body.
///
/// Assignments to property variables inside this block are stored under the combined
/// state mask formed by @p ... rather than the Default state. The previous state is
/// restored on scope exit, so blocks can be freely ordered without interfering.
/// More specific masks (more bits) win over less specific ones at resolve time.
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
            FUSION_FOLD_STATES(__VA_ARGS__)};                                          \
        true)

/// @brief Defines a stylesheet lambda compatible with FTheme::MergeStyleSheet().
/// The lambda receives an `FTheme* styleSheet` parameter used implicitly by FUSION_STYLE.
///
/// @example
/// theme->MergeStyleSheet(FUSION_STYLE_SHEET
/// {
///     FUSION_STYLE(FButton, "Button/Primary", Shape, Background, Border) { ... }
/// });
#define FUSION_STYLE_SHEET [](FTheme* styleSheet) -> void

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

    // -------------------------------------------------------------------------
    // FStyleProp — write proxy for a single named style property.
    // Holds a pointer to the shared context so state changes made by
    // FUSION_ON are visible through existing prop bindings.
    // operator= dispatches to the right FStyle setter by value type.
    // -------------------------------------------------------------------------

    struct FStyleProp
    {
        FStyleContext* m_Context;
        FName          m_Name;

        FStyleProp(FStyleContext* context, const FName& name)
            : m_Context(context), m_Name(name) {}

        FStyleProp& operator=(const FBrush& value)
        {
            m_Context->Style.Brush(m_Name, value, m_Context->State);
            return *this;
        }

        FStyleProp& operator=(const FPen& value)
        {
            m_Context->Style.Pen(m_Name, value, m_Context->State);
            return *this;
        }

        FStyleProp& operator=(const FShape& value)
        {
            m_Context->Style.Shape(m_Name, value, m_Context->State);
            return *this;
        }

        FStyleProp& operator=(const FColor& value)
        {
            m_Context->Style.Color(m_Name, value, m_Context->State);
            return *this;
        }

        FStyleProp& operator=(f32 value)
        {
            m_Context->Style.Float(m_Name, value, m_Context->State);
            return *this;
        }

        FStyleProp& operator=(const FVec2& value)
        {
            m_Context->Style.Vec2(m_Name, value, m_Context->State);
            return *this;
        }

        FStyleProp& operator=(const FVec4& value)
        {
            m_Context->Style.Vec4(m_Name, value, m_Context->State);
            return *this;
        }
    };

} // namespace Fusion::CSS

// -----------------------------------------------------------------------------
// __FUSION_CSS_MAKE_PROP — internal helper, produces one FStyleProp per name.
// Trailing comma is valid in a braced-init-list (C++11).
// -----------------------------------------------------------------------------

#define __FUSION_CSS_MAKE_PROP(PropName) \
    Fusion::CSS::FStyleProp{&_fusion_css_ctx, #PropName},

// -----------------------------------------------------------------------------
// FUSION_STYLE(WidgetClass, StyleName, ...Props)
//   WidgetClass — documents intent, unused in expansion
//   StyleName   — style registry key
//   ...Props    — property names brought into block scope via structured bindings
// -----------------------------------------------------------------------------

#define FUSION_STYLE(WidgetClass, StyleName, ...)                                      \
    if (Fusion::CSS::FStyleContext _fusion_css_ctx{styleSheet->Style(StyleName)};     \
        true)                                                                          \
    if (auto [__VA_ARGS__] = std::tuple{                                               \
            FUSION_MACRO_EXPAND(                                                       \
                FUSION_FOR_EACH(__FUSION_CSS_MAKE_PROP, __VA_ARGS__))                 \
        }; true)

// -----------------------------------------------------------------------------
// __FUSION_FOLD_STATES_N — folds state names with EStyleState:: prefix and |.
// -----------------------------------------------------------------------------

#define __FUSION_FOLD_STATES_1(a)          EStyleState::a
#define __FUSION_FOLD_STATES_2(a, b)       EStyleState::a | EStyleState::b
#define __FUSION_FOLD_STATES_3(a, b, c)    EStyleState::a | EStyleState::b | EStyleState::c
#define __FUSION_FOLD_STATES_4(a, b, c, d) EStyleState::a | EStyleState::b | EStyleState::c | EStyleState::d

#define FUSION_FOLD_STATES(...) \
    FUSION_MACRO_EXPAND(FUSION_CONCATENATE(__FUSION_FOLD_STATES_, FUSION_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__))

// -----------------------------------------------------------------------------
// FUSION_ON(...states)
//   Accepts one or more bare state names (Hovered, Pressed, etc.),
//   prefixes each with EStyleState:: and combines with |.
//   Mutates _fusion_css_ctx.State for the duration of the block.
// -----------------------------------------------------------------------------

#define FUSION_ON(...)                                                                 \
    if (Fusion::CSS::FStateScope _fusion_state_scope{                                 \
            _fusion_css_ctx,                                                           \
            FUSION_FOLD_STATES(__VA_ARGS__)};                                          \
        true)

#define FUSION_STYLE_SHEET [](FTheme* styleSheet) -> void

#pragma once

namespace Fusion
{

    enum class EStyleState : u32
    {
        Default = 0,
        Hovered = FUSION_BIT(0),
        Pressed = FUSION_BIT(1),
        Focused = FUSION_BIT(2),
        Disabled = FUSION_BIT(3),

        // Selection / toggle
        Selected = FUSION_BIT(4),  // tabs, list items, menu items, radio buttons
        Checked = FUSION_BIT(5),  // checkboxes, toggle switches

        // Input states
        ReadOnly = FUSION_BIT(6),  // editable=false but not grayed out like Disabled
        Error    = FUSION_BIT(7),  // validation failure — red border, etc.
        Warning  = FUSION_BIT(8),  // non-fatal issue — yellow border, etc.
        Editing  = FUSION_BIT(9),  // inline edit in progress (rename fields, cell editors)

        // Async / activity
        Loading  = FUSION_BIT(10), // async operation in progress — spinner, dimmed content

        // Activation
        Active   = FUSION_BIT(11), // current item among peers (active tab, active tool)

        // Structural states
        Expanded = FUSION_BIT(12), // tree nodes, accordions, dropdowns (collapsed = !Expanded)
    };
    FUSION_ENUM_CLASS_FLAGS(EStyleState);
    
} // namespace Fusion

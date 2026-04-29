#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    FItemView::FItemView()
    {
        m_ClipContent = true;
        m_ClipShape = FRectangle();
        m_ItemDelegate = CreateSubobject<FItemViewDelegate>();

        m_RowChevronSize = 10.0f;
        m_RowChevronGap = 6.0f;
        m_RowIconGap = 6.0f;
        m_RowIconWidth = 16.0f;
        m_RowLeftPadding = 4.0f;
        m_RowIndentWidth = 8.0f;
    }
}
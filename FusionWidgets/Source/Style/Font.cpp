#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    // - Implicit ctor -

    FFont::FFont(FName family, f32 pointSize, EFontWeight weight, EFontStyle style)
        : m_Family(family)
        , m_PointSize(pointSize)
        , m_Weight(weight)
        , m_Style(style)
    {
    }

    // - Named factories -

    FFont FFont::Regular(FName family, f32 pointSize)
    {
        return FFont(family, pointSize, EFontWeight::Regular, EFontStyle::Normal);
    }

    FFont FFont::Bold(FName family, f32 pointSize)
    {
        return FFont(family, pointSize, EFontWeight::Bold, EFontStyle::Normal);
    }

    FFont FFont::Light(FName family, f32 pointSize)
    {
        return FFont(family, pointSize, EFontWeight::Light, EFontStyle::Normal);
    }

    FFont FFont::Italic(FName family, f32 pointSize, EFontWeight weight)
    {
        return FFont(family, pointSize, weight, EFontStyle::Italic);
    }

    // - Equality -

    bool FFont::operator==(const FFont& rhs) const
    {
        return m_Family    == rhs.m_Family
            && m_PointSize == rhs.m_PointSize
            && m_Weight    == rhs.m_Weight
            && m_Style     == rhs.m_Style
            && m_Tracking  == rhs.m_Tracking
            && m_LineHeight == rhs.m_LineHeight;
    }

} // namespace Fusion

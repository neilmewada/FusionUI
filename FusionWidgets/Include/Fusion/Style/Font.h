#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    enum class EFontWeight : u32
    {
        Thin       = 100,
        ExtraLight = 200,
        Light      = 300,
        Regular    = 400,
        Medium     = 500,
        SemiBold   = 600,
        Bold       = 700,
        ExtraBold  = 800,
        Black      = 900,
    };
    FUSION_ENUM_CLASS(EFontWeight);

    enum class EFontStyle : u8
    {
        Normal,
        Italic,
    };
    FUSION_ENUM_CLASS(EFontStyle);

    class FUSIONWIDGETS_API FFont final
    {
    public:

        static constexpr auto kDefaultFamilyName = "Roboto";

        FFont() = default;

        // Implicit ctor — family + size covers the common case
        FFont(FName family, f32 pointSize,
              EFontWeight weight = EFontWeight::Regular,
              EFontStyle  style  = EFontStyle::Normal);

        // Named factories
        static FFont Regular  (FName family, f32 pointSize);
        static FFont Bold     (FName family, f32 pointSize);
        static FFont Light    (FName family, f32 pointSize);
        static FFont Italic   (FName family, f32 pointSize, EFontWeight weight = EFontWeight::Regular);

        // - Getters -

        FName       GetFamily()    const { return m_Family; }
        f32         GetPointSize() const { return m_PointSize; }
        EFontWeight GetWeight()    const { return m_Weight; }
        EFontStyle  GetStyle()     const { return m_Style; }
        f32         GetTracking()  const { return m_Tracking; }
        f32         GetLineHeight() const { return m_LineHeight; }

        bool IsValid() const { return m_Family.IsValid() && m_PointSize > 0.0f; }

        // - Fluent setters -

        FFont& Family(FName family)        { m_Family     = family;     return *this; }
        FFont& PointSize(f32 size)         { m_PointSize  = size;       return *this; }
        FFont& Weight(EFontWeight weight)  { m_Weight    = weight;     return *this; }
        FFont& Style(EFontStyle style)     { m_Style     = style;      return *this; }

        // Extra letter-spacing in pixels (positive = wider, negative = tighter).
        FFont& Tracking(f32 tracking)      { m_Tracking  = tracking;   return *this; }

        // Line height multiplier (1.0 = natural ascent+descent, 1.2 = 20% looser).
        FFont& LineHeight(f32 lineHeight)  { m_LineHeight = lineHeight; return *this; }

        bool operator==(const FFont& rhs) const;
        bool operator!=(const FFont& rhs) const { return !operator==(rhs); }

    private:

        FName m_Family;

        f32 m_PointSize  = 14.0f;
        f32 m_Tracking   = 0.0f;
        f32 m_LineHeight = 1.0f;

        EFontWeight m_Weight = EFontWeight::Regular;
        EFontStyle  m_Style  = EFontStyle::Normal;
    };

} // namespace Fusion

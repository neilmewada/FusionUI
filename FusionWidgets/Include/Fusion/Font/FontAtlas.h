#pragma once

namespace Fusion
{
    class FFont;

    struct FFontMetrics
    {
        f32 Ascender = 0;
        f32 Descender = 0;
        f32 LineGap = 0;
        f32 LineHeight = 0;
    };

    struct FGlyph
    {
        u32 CodePoint = 0;
        u32 AtlasLayerIndex = 0;
        u32 X = 0, Y = 0;
        u32 Width = 0, Height = 0;
        int BearingX = 0, BearingY = 0;
        int Advance = 0;

        u32 AtlasSize = 0;

        bool IsValid() const { return CodePoint > 0; }
    };

    class FUSIONWIDGETS_API FFontAtlas : public FObject
    {
        FUSION_CLASS(FFontAtlas, FObject)
    protected:

        FFontAtlas(Ref<FApplicationInstance> applicationInstance);

    public:

        static constexpr u32 kAtlasSize = 2048;
        static constexpr u32 kMaxLayers = 8;
        static constexpr u32 kSdfRenderSize = 64;
        static constexpr u32 kSdfPadding = 4;

        ~FFontAtlas();

        void Initialize();

        void Shutdown();

        FAtlasHandle GetHandle() const { return m_AtlasHandle; }

        void LoadFace(FName familyName, EFontWeight weight, EFontStyle style, const u8* data, SizeT dataSize);

        FFontMetrics GetScaledMetrics(const FFont& font);

        FGlyph FindOrAddGlyph(const FFont& font, u32 codePoint);

    private:

        struct FFontFaceKey
        {
            FName Family = "";
            EFontWeight Weight = EFontWeight::Regular;
            EFontStyle Style = EFontStyle::Normal;

            SizeT GetHash() const;

            bool operator==(const FFontFaceKey& rhs) const
            {
                return Family == rhs.Family && Weight == rhs.Weight && Style == rhs.Style;
            }

            bool operator!=(const FFontFaceKey& rhs) const { return !(*this == rhs); }
        };

        struct FFontGlyphKey
        {
            FName Family = "";
            EFontWeight Weight = EFontWeight::Regular;
            EFontStyle Style = EFontStyle::Normal;

            u32 CodePoint = 0;

            SizeT GetHash() const;

            bool operator==(const FFontGlyphKey& rhs) const
            {
                return Family == rhs.Family && Weight == rhs.Weight && Style == rhs.Style && CodePoint == rhs.CodePoint;
            }

            bool operator!=(const FFontGlyphKey& rhs) const { return !(*this == rhs); }
        };

        struct FFontFace
        {
            FFontFaceKey Key{};

            FT_Face Face = nullptr;

            FFontMetrics GetScaledMetrics(f32 pointSize) const
            {
                f32 scale = pointSize / (f32)Face->units_per_EM;
                f32 lineGap = Face->height - (Face->ascender - Face->descender);

                return FFontMetrics{
                    .Ascender   =  Face->ascender  * scale,
                    .Descender  =  Face->descender * scale,
                    .LineGap    =  lineGap    * scale,
                    .LineHeight =  (Face->ascender - Face->descender + lineGap) * scale,
                };
            }
        };

        struct FRowSegment {
            int X, Y;
            int Height;
        };

        struct FAtlasImageLayer : FIntrusiveBase
        {
            //u8* Ptr = nullptr;
            FArray<FRowSegment> Rows;

            bool TryInsertGlyph(FVec2i glyphSize, int& outX, int& outY);
        };

        void LoadGlyphs(FFontFaceKey key, const u32* codePoints, SizeT numCodePoints);

        WeakRef<FApplicationInstance> m_ApplicationInstance;
        FT_Library m_FtLibrary = nullptr;

        FFontFace m_DefaultFace{};

        FHashMap<FFontFaceKey, FFontFace> m_FontFaces;

        FArray<IPtr<FAtlasImageLayer>> m_AtlasImageLayers;
        int m_CurLayerIndex = -1;

        FHashMap<FFontGlyphKey, FGlyph> m_Glyphs;

        FAtlasHandle m_AtlasHandle;
    };
    
} // namespace Fusion

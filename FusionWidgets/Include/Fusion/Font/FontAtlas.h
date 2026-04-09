#pragma once

namespace Fusion
{
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
        u16 X = 0, Y = 0;
        u16 Width = 0, Height = 0;
        int BearingX = 0, BearingY = 0;
        int Advance = 0;
    };

    class FUSIONWIDGETS_API FFontAtlas : public FObject
    {
        FUSION_CLASS(FFontAtlas, FObject)
    protected:

        FFontAtlas(Ref<FApplicationInstance> applicationInstance);

    public:

        static constexpr u32 kAtlasSize = 2048;
        static constexpr u32 kMaxLayers = 8;
        static constexpr char* kDefaultFamilyName = "Roboto";

        ~FFontAtlas();

        void Initialize();

        void Shutdown();

        void LoadFace(FName familyName, EFontWeight weight, EFontStyle style, const u8* data, SizeT dataSize);

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

        void LoadGlyphs(FFontFaceKey key, const u32* codePoints, SizeT numCodePoints);

        WeakRef<FApplicationInstance> m_ApplicationInstance;
        FT_Library m_FtLibrary = nullptr;

        FFontFace m_DefaultFace{};

        FHashMap<FFontFaceKey, FFontFace> m_FontFaces;

        FAtlasHandle m_AtlasHandle;
    };
    
} // namespace Fusion

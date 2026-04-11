#include "Fusion/Widgets.h"
#include "Fusion/Resources/EmbeddedResources.h"

namespace Fusion
{
    static constexpr u32 kEnglishCodePoints[] = {
        // Printable ASCII: space through tilde (32–126)
        32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,   // space ! " # $ % & ' ( ) * + , - . /
        48,49,50,51,52,53,54,55,56,57,                      // 0–9
        58,59,60,61,62,63,64,                               // : ; < = > ? @
        65,66,67,68,69,70,71,72,73,74,75,76,77,             // A–M
        78,79,80,81,82,83,84,85,86,87,88,89,90,             // N–Z
        91,92,93,94,95,96,                                  // [ \ ] ^ _ `
        97,98,99,100,101,102,103,104,105,106,107,108,109,   // a–m
        110,111,112,113,114,115,116,117,118,119,120,121,122,// n–z
        123,124,125,126,                                    // { | } ~
    };

    FFontAtlas::FFontAtlas(Ref<FApplicationInstance> applicationInstance)
        : m_ApplicationInstance(applicationInstance)
    {
        IFRenderBackend* renderBackend = applicationInstance->GetRenderBackend();

        FT_Init_FreeType(&m_FtLibrary);

        m_AtlasHandle = renderBackend->CreateLayeredAtlas(true, kAtlasSize, 1);
    }

    FFontAtlas::~FFontAtlas()
    {
        Shutdown();
    }

    void FFontAtlas::Initialize()
    {
        const Resources::FResource* resource = Resources::Find("/Fonts/Roboto-Regular.ttf");
        FUSION_ASSERT(resource != nullptr, "Failed to find required resource: /Fonts/Roboto-Regular.ttf");

        m_AtlasImageLayers.Add(new FAtlasImageLayer);
        m_CurLayerIndex = 0;

        LoadFace(FFont::kDefaultFamilyName, EFontWeight::Regular, EFontStyle::Normal,
                 resource->Data, resource->Size);

        LoadGlyphs({.Family = FFont::kDefaultFamilyName, .Weight = EFontWeight::Regular, .Style = EFontStyle::Normal},
            kEnglishCodePoints, FUSION_COUNT(kEnglishCodePoints));
    }

    void FFontAtlas::Shutdown()
    {
        if (!m_FtLibrary)
            return;

        for (auto [_, face] : m_FontFaces)
        {
            FT_Done_Face(face.Face);
        }
        m_FontFaces.Clear();

        FT_Done_FreeType(m_FtLibrary);
        m_FtLibrary = nullptr;

        if (Ref<FApplicationInstance> applicationInstance = m_ApplicationInstance.Lock())
        {
            applicationInstance->GetRenderBackend()->DestroyAtlas(m_AtlasHandle);
            m_AtlasHandle = {};
        }
    }

    void FFontAtlas::LoadFace(FName familyName, EFontWeight weight, EFontStyle style, const u8* data, SizeT dataSize)
    {
        FFontFaceKey key = { familyName, weight, style };

        if (m_FontFaces.KeyExists(key))
            return;

        FT_Face ftFace = nullptr;
        FT_New_Memory_Face(m_FtLibrary, data, dataSize, 0, &ftFace);
        if (ftFace == nullptr)
            return;

        FFontFace face{
            .Key = key,
            .Face = ftFace
        };

        m_FontFaces[key] = face;
    }

    FFontMetrics FFontAtlas::GetScaledMetrics(const FFont& font)
    {
        FFontFaceKey key{
            .Family = font.GetFamily(),
            .Weight = font.GetWeight(),
            .Style = font.GetStyle()
        };

        auto it = m_FontFaces.Find(key);

        if (it == m_FontFaces.End())
            return {};

        return it->second.GetScaledMetrics(font.GetPointSize());
    }

    FGlyph FFontAtlas::FindOrAddGlyph(const FFont& font, u32 codePoint)
    {
        ZoneScoped;

        auto it = m_Glyphs.Find({
            .Family = font.GetFamily(),
            .Weight = font.GetWeight(),
            .Style = font.GetStyle(),
            .CodePoint = codePoint
        });

        if (it == m_Glyphs.End())
        {
            // TODO: Add this new glyph to the atlas
            return {};
        }

        return it->second;
    }

    SizeT FFontAtlas::FFontFaceKey::GetHash() const
    {
        SizeT hash = Family.GetHash();
        CombineHash(hash, (std::underlying_type_t<decltype(Weight)>)Weight);
        CombineHash(hash, (std::underlying_type_t<decltype(Style)>)Style);
        return hash;
    }

    SizeT FFontAtlas::FFontGlyphKey::GetHash() const
    {
        SizeT hash = Family.GetHash();
        CombineHash(hash, (std::underlying_type_t<decltype(Weight)>)Weight);
        CombineHash(hash, (std::underlying_type_t<decltype(Style)>)Style);
        CombineHash(hash, CodePoint);
        return hash;
    }

    bool FFontAtlas::FAtlasImageLayer::TryInsertGlyph(FVec2i glyphSize, int& outX, int& outY)
    {
        int bestRowIndex = -1;
        int bestRowHeight = INT_MAX;

        if (Rows.Empty())
        {
            Rows.Add({ .X = glyphSize.width, .Y = 0, .Height = glyphSize.height });
            outX = 0;
            outY = 0;
            return true;
        }

        for (int i = 0; i < Rows.Size(); i++)
        {
            int x = Rows[i].X;
            int y = Rows[i].Y;

            // Check if the glyph fits at this position
            if (x + glyphSize.width <= kAtlasSize && y + glyphSize.height <= kAtlasSize)
            {
                if (Rows[i].Height >= glyphSize.height && Rows[i].Height < bestRowHeight)
                {
                    bestRowHeight = Rows[i].Height;
                    bestRowIndex = i;
                }
            }
        }

        if (bestRowIndex == -1)
        {
            FRowSegment lastRow = Rows.Last();
            if (lastRow.Y + lastRow.Height + glyphSize.height > kAtlasSize)
            {
                return false;
            }

            Rows.Add({ .X = glyphSize.width, .Y = lastRow.Y + lastRow.Height, .Height = glyphSize.height });

            outX = 0;
            outY = Rows.Last().Y;

            return true;
        }

        outX = Rows[bestRowIndex].X;
        outY = Rows[bestRowIndex].Y;

        Rows[bestRowIndex].X += glyphSize.width;

        return true;
    }

    void FFontAtlas::LoadGlyphs(FFontFaceKey key, const u32* codePoints, SizeT numCodePoints)
    {
        Ref<FApplicationInstance> application = m_ApplicationInstance.Lock();
        if (!application)
            return;
        IFRenderBackend* renderBackend = application->GetRenderBackend();

        auto it = m_FontFaces.Find(key);
        if (it == m_FontFaces.End())
            return;

        FFontFace face = it->second;

        FT_Set_Pixel_Sizes(face.Face, 0, kSdfRenderSize);

        for (int i = 0; i < numCodePoints; i++)
        {
            const u32 codePoint = codePoints[i];

            FT_UInt glyphIndex = FT_Get_Char_Index(face.Face, codePoint);
            if (glyphIndex == 0)
                continue;

            if (FT_Load_Glyph(face.Face, glyphIndex, FT_LOAD_RENDER) != 0)
                continue;
            if (FT_Render_Glyph(face.Face->glyph, FT_RENDER_MODE_SDF) != 0)
                continue;

            FT_GlyphSlot slot = face.Face->glyph;

            FT_Bitmap& bitmap = slot->bitmap;

            FGlyph glyph{};
            glyph.CodePoint = codePoint;
            glyph.AtlasSize = kAtlasSize;
            glyph.Width = bitmap.width;
            glyph.Height = bitmap.rows;
            glyph.BearingX = slot->bitmap_left;
            glyph.BearingY = slot->bitmap_top;
            glyph.Advance = slot->advance.x >> 6;

            u8* pixels = bitmap.buffer;

            IPtr<FAtlasImageLayer> layer = m_AtlasImageLayers[m_CurLayerIndex];
            FVec2i glyphRectSize = FVec2i(glyph.Width + kSdfPadding, glyph.Height + kSdfPadding);
            int outX = 0, outY = 0;

            if (layer->TryInsertGlyph(glyphRectSize, outX, outY))
            {
                outX += kSdfPadding / 2;
                outY += kSdfPadding / 2;

                glyph.AtlasLayerIndex = m_CurLayerIndex;
                glyph.X = outX;
                glyph.Y = outY;

                m_Glyphs.Add({ .Family = key.Family, .Weight = key.Weight, .Style = key.Style, .CodePoint = codePoint }, glyph);

                if (pixels != nullptr)
                {
                    renderBackend->UploadAtlasRegionAsync(m_AtlasHandle, m_CurLayerIndex,
                        FVec2i(outX, outY),
                        FVec2i(bitmap.width, bitmap.rows),
                        pixels, bitmap.pitch);
                }
            }
            else
            {
	            // TODO: Allocate new layer and increment m_CurLayerIndex
            }
        }
    }
} // namespace Fusion

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

        LoadFace(kDefaultFamilyName, EFontWeight::Regular, EFontStyle::Normal,
            resource->Data, resource->Size);

        LoadGlyphs({.Family = kDefaultFamilyName, .Weight = EFontWeight::Regular, .Style = EFontStyle::Normal},
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

    SizeT FFontAtlas::FFontFaceKey::GetHash() const
    {
        SizeT hash = Family.GetHash();
        CombineHash(hash, (std::underlying_type_t<decltype(Weight)>)Weight);
        CombineHash(hash, (std::underlying_type_t<decltype(Style)>)Style);
        return hash;
    }

    void FFontAtlas::LoadGlyphs(FFontFaceKey key, const u32* codePoints, SizeT numCodePoints)
    {
        
    }
} // namespace Fusion

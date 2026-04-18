#pragma once
#include <Fusion/Misc/CoreTypes.h>

namespace Fusion
{
    enum class EImageFormat
    {
        Unknown,
        R8,
        RGBA8
    };

    struct FImageInfo
    {
        EImageFormat Format = EImageFormat::Unknown;
        int Width = 0;
        int Height = 0;
        int Channels = 0;
        int BitsPerPixel = 0;
    };

    inline int GetBitsPerPixel(EImageFormat format)
    {
        switch (format)
        {
        case EImageFormat::RGBA8:
            return 8 * 4;
        case EImageFormat::R8:
            return 8;
        case EImageFormat::Unknown:
            return 0;
        }
        return 0;
    }

    inline int GetNumChannels(EImageFormat format)
    {
        switch (format)
        {
        case EImageFormat::RGBA8:
            return 4;
        case EImageFormat::R8:
            return 1;
        case EImageFormat::Unknown:
            return 0;
        }
        return 0;
    }

    class FUSIONCORE_API FImage : public FImageInfo, public FIntrusiveBase
    {
    private:

        FImage() = default;

    public:

        ~FImage() override;

        static IPtr<FImage> Create(FName name, EImageFormat format, int width, int height);

        static IPtr<FImage> CreateExternal(FName name, EImageFormat format, int width, int height, u8* data);

        static IPtr<FImage> CreateFromMemory(FName name, const u8* data, SizeT size, int desiredChannels = 4);

        bool IsValid() const;

        u8* Data() const { return m_Data; }

        u32 Pitch() const { return Width * GetBitsPerPixel(Format) / 8; }

    private:

        FName m_Name;
        u8* m_Data = nullptr;
        bool m_UsesMalloc = false;
        bool m_UsesStbImage = false;
    };

} // namespace Fusion

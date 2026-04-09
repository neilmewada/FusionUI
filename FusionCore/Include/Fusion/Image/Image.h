#pragma once

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

        bool IsValid() const;

    private:

        FName m_Name;
        u8* m_Data = nullptr;
        bool m_UsesMalloc = false;
        bool m_UsesStbImage = false;
    };

} // namespace Fusion

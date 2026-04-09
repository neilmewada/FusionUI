#include "Fusion/Core.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Fusion
{
    FImage::~FImage()
    {
        if (m_UsesMalloc)
        {
            free(m_Data);
        }
        else if (m_UsesStbImage)
        {
            stbi_image_free(m_Data);
        }
    }


    IPtr<FImage> FImage::Create(FName name, EImageFormat format, int width, int height)
    {
        IPtr<FImage> image = new FImage();
        image->m_Name = MoveTemp(name);
        image->m_UsesMalloc = true;
        image->m_UsesStbImage = false;
        image->Width = width;
        image->Height = height;
        image->Channels = GetNumChannels(format);
        image->BitsPerPixel = GetBitsPerPixel(format);

        image->m_Data = (u8*)malloc((size_t)image->Width * (size_t)image->Height * image->BitsPerPixel / 8);

        return image;
    }

    bool FImage::IsValid() const
    {
        return m_Data != nullptr && Width > 0 && Height > 0 && Channels > 0;
    }

    
} // namespace Fusion

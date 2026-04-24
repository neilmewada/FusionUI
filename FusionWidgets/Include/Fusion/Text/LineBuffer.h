#pragma once

namespace Fusion
{

    class FUSIONWIDGETS_API FLineBuffer
    {
    public:

        void Insert(u32 lineNumber, u32 byteOffsetInLine, FStringView text);
        void Delete(u32 lineNumber, u32 byteOffsetInLine, u32 byteLength);

        FStringView GetLine(u32 lineIndex);
        u32 LineCount();
        u32 ByteLength();

    private:

        TArray<FString, 8> m_Lines;

        u32 m_TotalBytes = 0;


    };

} // namespace Fusion

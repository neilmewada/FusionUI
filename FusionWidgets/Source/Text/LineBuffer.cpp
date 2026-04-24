#include "Fusion/Widgets.h"

namespace Fusion
{
	void FLineBuffer::Insert(u32 lineNumber, u32 byteOffsetInLine, FStringView text)
	{
		if (lineNumber >= m_Lines.Size() || text.Empty())
			return;

		FString& line = m_Lines[lineNumber];

		byteOffsetInLine = FMath::Min<u32>(byteOffsetInLine, line.ByteLength());

		line.InsertBytes(byteOffsetInLine, text);

		m_TotalBytes += text.ByteLength();
	}

	void FLineBuffer::Delete(u32 lineNumber, u32 byteOffsetInLine, u32 byteLength)
	{
		if (lineNumber >= m_Lines.Size() || byteLength == 0)
			return;

		FString& line = m_Lines[lineNumber];

		if (byteOffsetInLine >= line.ByteLength())
			return;

		u32 actual = FMath::Min(byteLength, (u32)line.ByteLength() - byteOffsetInLine);
		line.EraseBytes(byteOffsetInLine, actual);
		m_TotalBytes -= actual;
	}

	FStringView FLineBuffer::GetLine(u32 lineIndex)
	{
		return m_Lines[lineIndex];
	}

	u32 FLineBuffer::ByteLength()
	{
		return m_TotalBytes;
	}

	u32 FLineBuffer::LineCount()
	{
		return m_Lines.Size();
	}

} // namespace Fusion

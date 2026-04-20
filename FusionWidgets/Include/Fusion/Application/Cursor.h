#pragma once

namespace Fusion
{
    enum class ECursorKind : u8
    {
	    Inherit = 0,
        System
    };
    FUSION_ENUM_CLASS(ECursorKind);

    class FUSIONWIDGETS_API FCursor
    {
    public:

        FCursor() = default;

        static FCursor Inherit()
        {
            FCursor c{};
            c.m_Kind = ECursorKind::Inherit;
            return c;
        }

        static FCursor System(ESystemCursor systemCursor)
        {
            FCursor c{};
            c.m_Kind = ECursorKind::System;
            c.m_SystemCursor = systemCursor;
            return c;
        }

        bool IsInherited() const { return m_Kind == ECursorKind::Inherit; }
        bool IsSystemCursor() const { return m_Kind == ECursorKind::System; }

        ECursorKind GetKind() const { return m_Kind; }
        ESystemCursor GetSystemCursor() const { return m_SystemCursor; }

    private:

        ECursorKind m_Kind = ECursorKind::Inherit;
        ESystemCursor m_SystemCursor = ESystemCursor::Default;

    };

} // namespace Fusion

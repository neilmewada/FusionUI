#include "Fusion/Widgets.h"

namespace Fusion
{
    FGradient FGradient::Linear(f32 angle, f32 startPoint, f32 endPoint)
    {
        FGradient g;
        g.m_Type       = EGradientType::Linear;
        g.m_Angle      = angle;
        g.m_StartPoint = startPoint;
        g.m_EndPoint   = endPoint;
        return g;
    }

    FGradient FGradient::Radial(FVec2 center, f32 radius)
    {
        FGradient g;
        g.m_Type   = EGradientType::Radial;
        g.m_Center = center;
        g.m_Radius = radius;
        return g;
    }

    FGradient FGradient::Conical(FVec2 center, f32 angle)
    {
        FGradient g;
        g.m_Type   = EGradientType::Conical;
        g.m_Center = center;
        g.m_Angle  = angle;
        return g;
    }

    FGradient& FGradient::AddStop(FColor color, f32 position)
    {
        m_Stops.Add({ color, position });
        return *this;
    }

    FGradient& FGradient::SetExtend(EGradientExtend extend)
    {
        m_Extend = extend;
        return *this;
    }

    bool FGradient::operator==(const FGradient& rhs) const
    {
        if (m_Type != rhs.m_Type || m_Extend != rhs.m_Extend)
            return false;

        if (m_Stops.Size() != rhs.m_Stops.Size())
            return false;

        for (SizeT i = 0; i < m_Stops.Size(); i++)
        {
            const FGradientStop& a = m_Stops[i];
            const FGradientStop& b = rhs.m_Stops[i];
            if (a.Color != b.Color || !FMath::ApproxEquals(a.Position, b.Position))
                return false;
        }

        switch (m_Type)
        {
        case EGradientType::Linear:
            return FMath::ApproxEquals(m_Angle,      rhs.m_Angle)      &&
                   FMath::ApproxEquals(m_StartPoint, rhs.m_StartPoint) &&
                   FMath::ApproxEquals(m_EndPoint,   rhs.m_EndPoint);

        case EGradientType::Radial:
            return m_Center == rhs.m_Center &&
                   FMath::ApproxEquals(m_Radius, rhs.m_Radius);

        case EGradientType::Conical:
            return m_Center == rhs.m_Center &&
                   FMath::ApproxEquals(m_Angle, rhs.m_Angle);
        }

        return false;
    }

} // namespace Fusion

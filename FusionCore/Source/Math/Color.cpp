#include "Fusion/Math/Color.h"

#include <cmath>

namespace Fusion
{

    FColor FColor::HSV(float h, float s, float v)
    {
        // h: [0, 360), s: [0, 1], v: [0, 1]
        if (s <= 0.0f)
            return { v, v, v, 1.0f };

        float hh = std::fmod(h, 360.0f) / 60.0f;
        int   i  = static_cast<int>(hh);
        float ff = hh - static_cast<float>(i);

        float p = v * (1.0f - s);
        float q = v * (1.0f - s * ff);
        float t = v * (1.0f - s * (1.0f - ff));

        switch (i)
        {
            case 0:  return { v, t, p, 1.0f };
            case 1:  return { q, v, p, 1.0f };
            case 2:  return { p, v, t, 1.0f };
            case 3:  return { p, q, v, 1.0f };
            case 4:  return { t, p, v, 1.0f };
            default: return { v, p, q, 1.0f };
        }
    }

} // namespace Fusion

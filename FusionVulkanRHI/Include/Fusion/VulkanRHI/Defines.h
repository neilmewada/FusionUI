#pragma once

namespace Fusion::Vulkan
{

    constexpr SizeT operator"" _KB(unsigned long long Value)
    {
        return Value * 1024;
    }

    constexpr SizeT operator"" _MB(unsigned long long Value)
    {
        return Value * (1024 * 1024);
    }
    
}

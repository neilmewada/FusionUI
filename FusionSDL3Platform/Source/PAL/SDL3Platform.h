#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FSDL3PlatformBase
    {
    public:
        FSDL3PlatformBase() = delete;
        ~FSDL3PlatformBase() = delete;
    };

} // namespace Fusion

#if FUSION_PLATFORM_WINDOWS
#include "Windows/SDL3WindowsPlatform.h"
#elif FUSION_PLATFORM_MAC
#include "Mac/SDL3MacPlatform.h"
#elif FUSION_PLATFORM_LINUX
#include "Linux/SDL3LinuxPlatform.h"
#endif

#pragma once

#include "Fusion/Misc/CoreDefines.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FException;
}

namespace Fusion::Internal
{
    // Constructs an FException with file/line context and throws it.
    // Never call directly — use FUSION_ASSERT.
    [[noreturn]] FUSIONCORE_API void AssertFailed(const char* condition, const char* message, const char* file, int line);

    template<typename TException> requires TFIsDerivedClass<FException, TException>::Value
    [[noreturn]] void AssertFailedThrow(const char* message, const char* file, int line);

} // namespace Fusion::Internal

#define FUSION_ASSERT(condition, message) \
    do { if (!(condition)) ::Fusion::Internal::AssertFailed(#condition, message, __FILE__, __LINE__); } while(0)

#define FUSION_ASSERT_THROW(condition, TException, message) \
    do { if (!(condition)) ::Fusion::Internal::AssertFailedThrow<TException>(message, __FILE__, __LINE__); } while(0)

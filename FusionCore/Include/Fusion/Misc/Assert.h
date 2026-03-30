#pragma once

#include "Fusion/Misc/CoreDefines.h"

namespace Fusion::Internal
{
    // Constructs an FException with file/line context and throws it.
    // Never call directly — use FUSION_ASSERT.
    [[noreturn]] FUSIONCORE_API void AssertFailed(const char* condition, const char* message, const char* file, int line);

} // namespace Fusion::Internal

#define FUSION_ASSERT(condition, message) \
    do { if (!(condition)) ::Fusion::Internal::AssertFailed(#condition, message, __FILE__, __LINE__); } while(0)

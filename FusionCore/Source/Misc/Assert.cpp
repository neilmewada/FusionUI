#include "Fusion/Misc/Assert.h"
#include "Fusion/Misc/Exception.h"
#include "Fusion/Containers/String.h"

namespace Fusion::Internal
{
    [[noreturn]] void AssertFailed(const char* condition, const char* message, const char* file, int line)
    {
        FString fullMessage = FString(file) + ":" + std::to_string(line) + ": Assertion failed: (" + condition + ") — " + message;
        throw FException(fullMessage);
    }

} // namespace Fusion::Internal

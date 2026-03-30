#include "Fusion/Log/Log.h"

#include <cstdio>

namespace Fusion
{

    static FLogSink s_Sink = nullptr;

    static const char* LevelToString(FLogLevel level)
    {
        switch (level)
        {
            case FLogLevel::Trace:    return "Trace";
            case FLogLevel::Info:     return "Info";
            case FLogLevel::Warning:  return "Warning";
            case FLogLevel::Error:    return "Error";
            case FLogLevel::Critical: return "Critical";
            default:                  return "Unknown";
        }
    }

    void SetLogSink(FLogSink sink)
    {
        s_Sink = sink;
    }

    void Log(FLogLevel level, const char* category, const char* message)
    {
        if (s_Sink)
        {
            s_Sink(level, category, message);
        }
        else
        {
            std::fprintf(stderr, "[Fusion][%s][%s] %s\n", LevelToString(level), category, message);
        }
    }

} // namespace Fusion

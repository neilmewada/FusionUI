#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#include "Containers/String.h"

#include <exception>
#include "cpptrace/cpptrace.hpp"

#if FUSION_EXCEPTIONS

#define FUSION_TRY try
#define FUSION_CATCH(exceptionStatement) catch (exceptionStatement)

#else

#define FUSION_TRY if constexpr (true)
#define FUSION_CATCH(exceptionStatement) else if constexpr (false)

#endif

namespace Fusion
{

    class FUSIONCORE_API FException : public std::exception
    {
    public:

        FException() : message("Unknown error"), stackTrace(cpptrace::generate_trace())
        {
	        
        }

        FException(const FString& message) : message(message), stackTrace(cpptrace::generate_trace())
        {
	        
        }

        const char* what() const throw () 
    	{
            return message.CStr();
        }

        const cpptrace::stacktrace& GetStackTrace() const
        {
            return stackTrace;
		}

        FString GetStackTraceString(bool useColors) const
        {
            return FString(stackTrace.to_string(useColors));
		}

    private:

        FString message;
        cpptrace::stacktrace stackTrace;
    };

    class FUSIONCORE_API FNullPointerException : public FException
    {
    public:

        FNullPointerException() : FException("NullPointerException")
        {
	        
        }

        FNullPointerException(const FString& message) : FException(FString::Format("NullPointerException: {}", message))
        {
	        
        }

    private:


    };

    class FUSIONCORE_API FOutOfBoundsException : public FException
    {
    public:

        FOutOfBoundsException() : FException("OutOfBoundsException")
        {

        }

        FOutOfBoundsException(const FString& message) : FException(FString::Format("OutOfBoundsException: {}", message))
        {

        }

    private:


    };

    namespace Internal
    {
        template<typename TException> requires TFIsDerivedClass<FException, TException>::Value
        [[noreturn]] void AssertFailedThrow(const char* message, const char* file, int line)
        {
            FString fullMessage = FString(file) + ":" + std::to_string(line) + " - " + message;
#if FUSION_EXCEPTIONS
            throw TException(fullMessage);
#else
            FUSION_LOG_CRITICAL("Assert", fullMessage.CStr());
            assert(false && fullMessage.CStr());
#endif
        }
    }

} // namespace Fusion

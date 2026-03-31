#pragma once

#include "Fusion/Misc/CoreTypes.h"

namespace Fusion 
{

	enum class FShaderStage : u8
	{
	    Vertex,
	    Fragment,
	    Compute,
	    Geometry,
	    TessControl,
	    TessEval,
	};

	// A single compiled stage within a shader (e.g. the vertex stage of "Fusion").
	// All pointer members are non-owning; data lives in static storage.
	struct FShaderModule
	{
	    FShaderStage Stage      = FShaderStage::Vertex;
	    const char*  EntryPoint = nullptr;

	    // SPIR-V bytecode for Vulkan.
	    // Aligned to 4 bytes; size is in bytes.
	    // Cast to const u32* when passing to vkCreateShaderModule.
	    const u8*    SPIRVData  = nullptr;
	    SizeT        SPIRVSize  = 0;

	    // Reserved for Metal (MSL). Null until Metal support is enabled.
	    const char*  MSLSource     = nullptr;
	    SizeT        MSLSourceSize = 0;
	};

	// A named shader grouping one or more stage modules.
	// Graphics shaders have a vertex + fragment module; compute shaders have one.
	struct FShader
	{
	    const char*          Name        = nullptr;
	    const FShaderModule* Modules     = nullptr;
	    SizeT                ModuleCount = 0;

	    const FShaderModule* FindModule(FShaderStage stage) const
	    {
	        for (SizeT i = 0; i < ModuleCount; ++i)
	            if (Modules[i].Stage == stage) return &Modules[i];
	        return nullptr;
	    }
	};

	// Lightweight non-owning view over the static shader table.
	// Supports range-for and index access.
	struct FShaderView
	{
	    const FShader* Data  = nullptr;
	    SizeT          Count = 0;

	    const FShader* begin() const { return Data; }
	    const FShader* end()   const { return Data + Count; }
	    SizeT          size()  const { return Count; }
	    bool           empty() const { return Count == 0; }
	    const FShader& operator[](SizeT i) const { return Data[i]; }
	};

} // namespace Fusion

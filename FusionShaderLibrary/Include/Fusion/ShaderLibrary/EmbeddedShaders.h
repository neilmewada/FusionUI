#pragma once

#include "Fusion/ShaderLibrary/ShaderTypes.h"

// ---------------------------------------------------------------------------
// Fusion::Shaders -- access to precompiled, embedded shader data.
//
// All() and FindShader() are implemented in the auto-generated
// ShaderRegistry.cpp (produced by fusion_finalize_shader_registry in CMake).
// ---------------------------------------------------------------------------

namespace Fusion::Shaders {

// All shaders compiled into the library, in registration order.
// Supports range-for:
//   for (const FShader& shader : Fusion::Shaders::All()) { ... }
FShaderView All();

// Find a shader by name. Returns nullptr if not found.
const FShader* FindShader(const char* name);

} // namespace Fusion::Shaders

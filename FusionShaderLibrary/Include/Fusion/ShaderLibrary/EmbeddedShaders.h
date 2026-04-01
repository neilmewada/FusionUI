#pragma once

#include "Fusion/ShaderLibrary/ShaderTypes.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion::Shaders {

	// All shaders compiled for the given format.
	// A shader only appears here if every one of its stage modules was compiled
	// for that format -- so the returned set is always complete and usable.
	//
	//   for (const FShader& shader : Fusion::Shaders::All(FShaderFormat::SPIRV)) { ... }
	//
	FShaderLibrary All();

	// Find a shader by name within a format set.
	// Returns nullptr if the shader does not exist or was not compiled for that format.
	const FShader* FindShader(const char* name);

} // namespace Fusion::Shaders

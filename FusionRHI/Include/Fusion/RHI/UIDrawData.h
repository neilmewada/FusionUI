#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	using FUIIndex = u32;

	enum class EUIShaderType : u32
	{
		SolidColor = 0,
		Texture,
		LinearGradient,
		RadialGradient,
		ConicGradient,
		SDFText,
		Custom
	};
	FUSION_ENUM_CLASS(EUIShaderType);

	enum class EUIDrawItemFlags : u32
	{
		None = 0,
		TextureTileX = 1 << 0,
		TextureTileY = 1 << 1,
		ImageFitCover = 1 << 2,
		ImageFitContain = 1 << 3,
	};
	FUSION_ENUM_CLASS_FLAGS(EUIDrawItemFlags);

	enum class EUIBlendMode : u32
	{
		Normal = 0, // Standard alpha-over
		Additive,
		Multiply
	};
	FUSION_ENUM_CLASS(EUIBlendMode);

	struct FUIVertex
	{
		FVec2 pos;
		FVec2 uv;
		u32 color = 0;
		u32 drawItemIndex = 0;
	};

	struct FUIQuadVertex
	{
		FVec2 pos;
		FVec2 uv;
	};

	struct alignas(16) FUIClipRect
	{
		FMat4 clipInverseTransform;
		FVec4 cornerRadii;
		FVec2 clipHalfSize;
		int   parentClipIndex = -1;  // -1 = no parent (root clip)
		int   _pad;
	};

	struct FUIGradientStop
	{
		u32 packedColor = 0;
		f32 position = 0;
	};

	struct alignas(4) FUIDrawItem
	{
		EUIShaderType shaderType = EUIShaderType::SolidColor;
		u32 textureIndex = 0;
		u32 samplerIndex = 0;
		EUIDrawItemFlags drawItemFlags = EUIDrawItemFlags::None;

		int clipRectIndex = -1;
		int gradientStartIndex = 0;
		int gradientStopCount = 0;
		u32 userFlags = 0;

		// 128 bytes: per-shader payload
		f32 data[32] = {};
	};
	static_assert(sizeof(FUIDrawItem) == 160);

	struct FUIDrawCmd
	{
		u32 IndexOffset = 0;
		u32 IndexCount = 0;
		u32 VertexOffset = 0;

		EUIBlendMode BlendMode = EUIBlendMode::Normal;
	};

	struct FSplitRange
	{
		SizeT StartOffset = 0;
		SizeT ByteSize = 0;
	};

	struct alignas(4) FUIViewData
	{
		FMat4 ViewMatrix;
		FMat4 ViewProjectionMatrix;
		FMat4 ProjectionMatrix;
		FVec4 ViewPosition;
		FVec2 PixelResolution;

		u32   _pad[10];
	};
	static_assert(sizeof(FUIViewData) == 256);
    
} // namespace Fusion

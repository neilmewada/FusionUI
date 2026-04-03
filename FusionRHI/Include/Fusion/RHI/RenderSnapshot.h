#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	FUSION_DEFINE_HANDLE_TYPE(FRenderTargetHandle, u32);

	struct FRenderPass
	{
		FRenderTargetHandle Handle;

		SizeT LayerIndex = 0;
		SizeT DrawCmdStartIndex = 0;
		SizeT DrawCmdCount = 0;
	};

    class FUSIONRHI_API FRenderSnapshot : public FIntrusiveBase
    {
    public:

		FRenderSnapshot() = default;

		using FSplitRangeArray = FStableDynamicArray<FSplitRange, 64>;
		using FRenderPassArray = FStableDynamicArray<FRenderPass, 32>;

		FUIVertexArray vertexArray;
		FSplitRangeArray vertexSplits;

		FUIIndexArray indexArray;
		FSplitRangeArray indexSplits;

		FUIDrawItemArray drawItemArray;
		FSplitRangeArray drawItemSplits;

		FUIDrawCmdArray drawCmdArray;
		FSplitRangeArray drawCmdSplits;

		FUIClipRectArray clipRectArray;
		FSplitRangeArray clipRectSplits;

		FUIGradientStopArray gradientStopArray;
		FSplitRangeArray gradientStopSplits;

		//! @brief This array defines the actual order of draw commands.
		FRenderPassArray renderPassArray;

		FUIViewData viewData;

		FUIMatrixArray transformMatricesPerLayer;

    };
    
} // namespace Fusion


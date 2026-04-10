#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    using FUIVertexArray        = FStableGrowthArray<FUIVertex,        1024>;
    using FUIIndexArray         = FStableGrowthArray<FUIIndex,         1024>;
    using FUIDrawItemArray      = FStableGrowthArray<FUIDrawItem,      512>;
    using FUIClipRectArray      = FStableGrowthArray<FUIClipRect,      64>;
    using FUIGradientStopArray  = FStableGrowthArray<FUIGradientStop,  64>;
    using FUIDrawCmdArray       = FStableGrowthArray<FUIDrawCmd,       64>;
    using FUIMatrixArray	    = FStableGrowthArray<FMat4,            64>;

	class FUSIONRHI_API FUIDrawList final
	{
	public:

        // -  Public API -

        void Clear();
        void Finalize();

        SizeT GetCurrentDrawCmdCount() const { return drawCmdArray.GetCount(); }

        // Current vertex count — used to compute relative index offsets
        u32 GetCurrentVertexCount() const { return vertexArray.GetCount(); }

        FUIDrawCmd& NewDrawCmd();

        FUIDrawCmd& AcquireDrawCmd();

        u32 AddDrawItem(const FUIDrawItem& item);

        void AddPolyLine(const FVec2* points, int numPoints, u32 color, f32 thickness, bool closed, bool antiAliased, u32 drawItemIndex = 0, const f32* uvXValues = nullptr, const FRect* minMaxPos = nullptr, f32 gradientOffset = 0.0f);

        void AddConvexPolyFilled(const FVec2* points, int numPoints, u32 color, bool antiAliased, FRect* minMaxPos, u32 drawItemIndex = 0);

        void AddQuad(const FRect& rect, FVec2 uvMin, FVec2 uvMax, u32 color, u32 drawItemIndex = 0);

        void PrimReserve(int vertexCount, int indexCount);

        // - Types & Constants -

        using FTempPointsArray = FStableGrowthArray<FVec2, 128>;
        static constexpr u32 ColorAlphaMask = 0xff000000;

        // - Data -

        FUIVertexArray vertexArray;
        FUIIndexArray indexArray;
        FUIDrawItemArray drawItemArray;
        FUIClipRectArray clipRectArray;
        FUIGradientStopArray gradientStopArray;
        FUIDrawCmdArray drawCmdArray;

        FTempPointsArray temporaryPoints;
        FUIVertex* vertexWritePtr = nullptr;
        FUIIndex* indexWritePtr = nullptr;

        // Start offset of current vertex
        FUIIndex vertexCurrentIdx = 0;

        float fringeScale = 1.0f;

        EUIBlendMode blendMode = EUIBlendMode::Normal;


	};

}

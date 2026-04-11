#include "Fusion/RHI.h"

// Credit: Dear Imgui
#define IM_NORMALIZE2F_OVER_ZERO(VX,VY)     { float d2 = VX*VX + VY*VY; if (d2 > 0.0f) { float inv_len = ImRsqrt(d2); VX *= inv_len; VY *= inv_len; } } (void)0
#define IM_FIXNORMAL2F_MAX_INVLEN2          100.0f // 500.0f (see #4053, #3366)
#define IM_FIXNORMAL2F(VX,VY)               { float d2 = VX*VX + VY*VY; if (d2 > 0.000001f) { float inv_len2 = 1.0f / d2; if (inv_len2 > IM_FIXNORMAL2F_MAX_INVLEN2) inv_len2 = IM_FIXNORMAL2F_MAX_INVLEN2; VX *= inv_len2; VY *= inv_len2; } } (void)0
static inline float ImRsqrt(float x) { return 1.0f / sqrtf(x); }

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    void FUIDrawList::Clear()
    {
        ZoneScoped;

        vertexArray.RemoveAll();
        indexArray.RemoveAll();
        drawItemArray.RemoveAll();
        clipRectArray.RemoveAll();
        gradientStopArray.RemoveAll();
        drawCmdArray.RemoveAll();

        vertexWritePtr = nullptr;
        indexWritePtr = nullptr;
        vertexCurrentIdx = 0;

        // First draw item is always the default one
        drawItemArray.Insert(FUIDrawItem{
            .shaderType = EUIShaderType::SolidColor,
            .textureIndex = 0,
            .samplerIndex = 0,
            .drawItemFlags = EUIDrawItemFlags::None,
            .clipRectIndex = 0,//-1,
            .gradientStartIndex = 0,
            .gradientStopCount = 0,
            .userFlags = 0
        });
    }

    void FUIDrawList::Finalize()
    {
        // indexCount is maintained incrementally by prim functions (e.g. PrimRect).
        // Nothing to do here.
    }

    u32 FUIDrawList::AddDrawItem(const FUIDrawItem& item)
    {
        u32 index = (u32)drawItemArray.GetCount();
        drawItemArray.Insert(item);
        return index;
    }

    FUIDrawCmd& FUIDrawList::NewDrawCmd()
    {
        if (!drawCmdArray.IsEmpty() && drawCmdArray.Last().IndexCount == 0)
            return drawCmdArray.Last(); // already a clean boundary, reuse it

        FUIDrawCmd cmd{};
        cmd.IndexOffset = (u32)indexArray.GetCount();
        cmd.VertexOffset = 0;
        cmd.BlendMode = drawCmdArray.IsEmpty() ? EUIBlendMode::Normal : drawCmdArray.Last().BlendMode;
        drawCmdArray.Insert(cmd);

        return drawCmdArray.Last();
    }

    FUIDrawCmd& FUIDrawList::AcquireDrawCmd()
    {
        if (!drawCmdArray.IsEmpty())
        {
            auto& last = drawCmdArray.Last();
            if (last.BlendMode == blendMode)
            {
                return last;
            }

            if (last.IndexCount == 0)
            {
                // Last command had nothing written — reuse the slot with the new state
                last.BlendMode = blendMode;
                return last;
            }
        }

        FUIDrawCmd cmd{};
        cmd.IndexOffset = (u32)indexArray.GetCount();
        cmd.VertexOffset = 0;
        cmd.BlendMode = blendMode;
        drawCmdArray.Insert(cmd);

        return drawCmdArray.Last();
    }

    void FUIDrawList::PrimReserve(int vertexCount, int indexCount)
    {
        ZoneScoped;

        SizeT curVertexCount = vertexArray.GetCount();

        vertexArray.InsertRange(vertexCount);
        vertexWritePtr = vertexArray.GetData() + curVertexCount;

        SizeT curIndexCount = indexArray.GetCount();

        indexArray.InsertRange(indexCount);
        indexWritePtr = indexArray.GetData() + curIndexCount;
    }

    void FUIDrawList::AddPolyLine(const FVec2* points, int numPoints, u32 color, f32 thickness, bool closed, bool antiAliased, u32 drawItemIndex, const f32* uvXValues, const FRect* minMaxPos, f32 gradientOffset)
    {
        ZoneScoped;

        if (points == nullptr || numPoints <= 0)
            return;
        if ((color & ColorAlphaMask) == 0)
            return;

        auto getUV = [&](int i, FVec2 pos) -> FVec2 {
            return uvXValues 
        		? FVec2(uvXValues[i], 0.5f) 
        		: minMaxPos ? (pos - minMaxPos->min) / minMaxPos->GetSize() : FVec2(0, 0);
        };

        const int count = closed ? numPoints : numPoints - 1; // The number of line segments we need to draw
        const bool thickLine = (thickness > fringeScale);

        FUIDrawCmd& drawCmd = AcquireDrawCmd();

        if (antiAliased)
        {
            // [PATH 1]
            // Anti-aliased stroke
            const float AA_SIZE = fringeScale;
            const u32 transparentColor = color & ~ColorAlphaMask;

            // Thicknesses <1.0 should behave like thickness 1.0
            thickness = FMath::Max(thickness, 1.0f);

            const int indexCount = (thickLine ? count * 18 : count * 12);
            const int vertexCount = (thickLine ? numPoints * 4 : numPoints * 3);
            PrimReserve(vertexCount, indexCount);

            // Temporary buffer
            // The first <numPoints> items are normals at each line point, then after that there are either 2 or 4 temp points for each line point
            temporaryPoints.RemoveAll();
            temporaryPoints.InsertRange(numPoints * (thickLine ? 5 : 3));
            FVec2* tempNormals = temporaryPoints.GetData();
            FVec2* tempPoints = tempNormals + numPoints;

            // Calculate normals (tangents) for each line segment
            for (int i1 = 0; i1 < count; i1++)
            {
                const int i2 = (i1 + 1) == numPoints ? 0 : i1 + 1;
                float dx = points[i2].x - points[i1].x;
                float dy = points[i2].y - points[i1].y;
                IM_NORMALIZE2F_OVER_ZERO(dx, dy);
                tempNormals[i1].x = dy;
                tempNormals[i1].y = -dx;
            }

            if (!closed)
                tempNormals[numPoints - 1] = tempNormals[numPoints - 2];

            if (!thickLine)
            {
                const float half_draw_size = AA_SIZE;

                // If line is not closed, the first and last points need to be generated differently as there are no normals to blend
                if (!closed)
                {
                    tempPoints[0] = points[0] + tempNormals[0] * half_draw_size;
                    tempPoints[1] = points[0] - tempNormals[0] * half_draw_size;
                    tempPoints[(numPoints - 1) * 2 + 0] = points[numPoints - 1] + tempNormals[numPoints - 1] * half_draw_size;
                    tempPoints[(numPoints - 1) * 2 + 1] = points[numPoints - 1] - tempNormals[numPoints - 1] * half_draw_size;
                }

                // Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
                // This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
                // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
                unsigned int idx1 = vertexCurrentIdx; // Vertex index for start of line segment
                for (int i1 = 0; i1 < count; i1++) // i1 is the first point of the line segment
                {
                    const int i2 = (i1 + 1) == numPoints ? 0 : i1 + 1; // i2 is the second point of the line segment
                    const unsigned int idx2 = ((i1 + 1) == numPoints) ? vertexCurrentIdx : (idx1 + 3); // Vertex index for end of segment

                    // Average normals
                    float dm_x = (tempNormals[i1].x + tempNormals[i2].x) * 0.5f;
                    float dm_y = (tempNormals[i1].y + tempNormals[i2].y) * 0.5f;
                    IM_FIXNORMAL2F(dm_x, dm_y);
                    dm_x *= half_draw_size; // dm_x, dm_y are offset to the outer edge of the AA area
                    dm_y *= half_draw_size;

                    // Add temporary vertexes for the outer edges
                    FVec2* out_vtx = &tempPoints[i2 * 2];
                    out_vtx[0].x = points[i2].x + dm_x;
                    out_vtx[0].y = points[i2].y + dm_y;
                    out_vtx[1].x = points[i2].x - dm_x;
                    out_vtx[1].y = points[i2].y - dm_y;

                    {
                        // Add indexes for four triangles
                        indexWritePtr[0] = (FUIIndex)(idx2 + 0); indexWritePtr[1] = (FUIIndex)(idx1 + 0); indexWritePtr[2] = (FUIIndex)(idx1 + 2);  // Right tri 1
                        indexWritePtr[3] = (FUIIndex)(idx1 + 2); indexWritePtr[4] = (FUIIndex)(idx2 + 2); indexWritePtr[5] = (FUIIndex)(idx2 + 0);  // Right tri 2
                        indexWritePtr[6] = (FUIIndex)(idx2 + 1); indexWritePtr[7] = (FUIIndex)(idx1 + 1); indexWritePtr[8] = (FUIIndex)(idx1 + 0);  // Left tri 1
                        indexWritePtr[9] = (FUIIndex)(idx1 + 0); indexWritePtr[10] = (FUIIndex)(idx2 + 0); indexWritePtr[11] = (FUIIndex)(idx2 + 1);// Left tri 2
                        indexWritePtr += 12;
                    }

                    idx1 = idx2;
                }

                {
                    // If we're not using a texture, we need the center vertex as well
                    for (int i = 0; i < numPoints; i++)
                    {
                        const FVec2 uv = getUV(i, points[i]);
                        vertexWritePtr[0].pos = points[i];             vertexWritePtr[0].uv = uv; vertexWritePtr[0].color = color;            vertexWritePtr[0].drawItemIndex = drawItemIndex; // Center of line
                        vertexWritePtr[1].pos = tempPoints[i * 2 + 0]; vertexWritePtr[1].uv = uv; vertexWritePtr[1].color = transparentColor; vertexWritePtr[1].drawItemIndex = drawItemIndex; // Left-side outer edge
                        vertexWritePtr[2].pos = tempPoints[i * 2 + 1]; vertexWritePtr[2].uv = uv; vertexWritePtr[2].color = transparentColor; vertexWritePtr[2].drawItemIndex = drawItemIndex; // Right-side outer edge
                        vertexWritePtr += 3;
                    }
                }
            }
            else
            {
                // [PATH 2] Non texture-based lines (thick): we need to draw the solid line core and thus require four vertices per point
                const float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;

                // If line is not closed, the first and last points need to be generated differently as there are no normals to blend
                if (!closed)
                {
                    const int points_last = numPoints - 1;
                    tempPoints[0] = points[0] + tempNormals[0] * (half_inner_thickness + AA_SIZE);
                    tempPoints[1] = points[0] + tempNormals[0] * (half_inner_thickness);
                    tempPoints[2] = points[0] - tempNormals[0] * (half_inner_thickness);
                    tempPoints[3] = points[0] - tempNormals[0] * (half_inner_thickness + AA_SIZE);
                    tempPoints[points_last * 4 + 0] = points[points_last] + tempNormals[points_last] * (half_inner_thickness + AA_SIZE);
                    tempPoints[points_last * 4 + 1] = points[points_last] + tempNormals[points_last] * (half_inner_thickness);
                    tempPoints[points_last * 4 + 2] = points[points_last] - tempNormals[points_last] * (half_inner_thickness);
                    tempPoints[points_last * 4 + 3] = points[points_last] - tempNormals[points_last] * (half_inner_thickness + AA_SIZE);
                }

                // Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
                // This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
                // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
                unsigned int idx1 = vertexCurrentIdx; // Vertex index for start of line segment
                for (int i1 = 0; i1 < count; i1++) // i1 is the first point of the line segment
                {
                    const int i2 = (i1 + 1) == numPoints ? 0 : (i1 + 1); // i2 is the second point of the line segment
                    const unsigned int idx2 = (i1 + 1) == numPoints ? vertexCurrentIdx : (idx1 + 4); // Vertex index for end of segment

                    // Average normals
                    float dm_x = (tempNormals[i1].x + tempNormals[i2].x) * 0.5f;
                    float dm_y = (tempNormals[i1].y + tempNormals[i2].y) * 0.5f;
                    IM_FIXNORMAL2F(dm_x, dm_y);
                    float dm_out_x = dm_x * (half_inner_thickness + AA_SIZE);
                    float dm_out_y = dm_y * (half_inner_thickness + AA_SIZE);
                    float dm_in_x = dm_x * half_inner_thickness;
                    float dm_in_y = dm_y * half_inner_thickness;

                    // Add temporary vertices
                    FVec2* out_vtx = &tempPoints[i2 * 4];
                    out_vtx[0].x = points[i2].x + dm_out_x;
                    out_vtx[0].y = points[i2].y + dm_out_y;
                    out_vtx[1].x = points[i2].x + dm_in_x;
                    out_vtx[1].y = points[i2].y + dm_in_y;
                    out_vtx[2].x = points[i2].x - dm_in_x;
                    out_vtx[2].y = points[i2].y - dm_in_y;
                    out_vtx[3].x = points[i2].x - dm_out_x;
                    out_vtx[3].y = points[i2].y - dm_out_y;

                    // Add indexes
                    indexWritePtr[0] = (FUIIndex)(idx2 + 1); indexWritePtr[1] = (FUIIndex)(idx1 + 1); indexWritePtr[2] = (FUIIndex)(idx1 + 2);
                    indexWritePtr[3] = (FUIIndex)(idx1 + 2); indexWritePtr[4] = (FUIIndex)(idx2 + 2); indexWritePtr[5] = (FUIIndex)(idx2 + 1);
                    indexWritePtr[6] = (FUIIndex)(idx2 + 1); indexWritePtr[7] = (FUIIndex)(idx1 + 1); indexWritePtr[8] = (FUIIndex)(idx1 + 0);
                    indexWritePtr[9] = (FUIIndex)(idx1 + 0); indexWritePtr[10] = (FUIIndex)(idx2 + 0); indexWritePtr[11] = (FUIIndex)(idx2 + 1);
                    indexWritePtr[12] = (FUIIndex)(idx2 + 2); indexWritePtr[13] = (FUIIndex)(idx1 + 2); indexWritePtr[14] = (FUIIndex)(idx1 + 3);
                    indexWritePtr[15] = (FUIIndex)(idx1 + 3); indexWritePtr[16] = (FUIIndex)(idx2 + 3); indexWritePtr[17] = (FUIIndex)(idx2 + 2);
                    indexWritePtr += 18;

                    idx1 = idx2;
                }

                // Add vertices
                for (int i = 0; i < numPoints; i++)
                {
                    const FVec2 uv = getUV(i, tempPoints[i * 4 + 1]);
                    vertexWritePtr[0].pos = tempPoints[i * 4 + 0]; vertexWritePtr[0].uv = uv; vertexWritePtr[0].color = transparentColor; vertexWritePtr[0].drawItemIndex = drawItemIndex;
                    vertexWritePtr[1].pos = tempPoints[i * 4 + 1]; vertexWritePtr[1].uv = uv; vertexWritePtr[1].color = color;            vertexWritePtr[1].drawItemIndex = drawItemIndex;
                    vertexWritePtr[2].pos = tempPoints[i * 4 + 2]; vertexWritePtr[2].uv = uv; vertexWritePtr[2].color = color;            vertexWritePtr[2].drawItemIndex = drawItemIndex;
                    vertexWritePtr[3].pos = tempPoints[i * 4 + 3]; vertexWritePtr[3].uv = uv; vertexWritePtr[3].color = transparentColor; vertexWritePtr[3].drawItemIndex = drawItemIndex;
                    vertexWritePtr += 4;
                }
            }

            vertexCurrentIdx += vertexCount;
            drawCmd.IndexCount += indexCount;
        }
        else
        {
            // [PATH 4] Non texture-based, Non anti-aliased lines
            const int indexCount = count * 6;
            const int vertexCount = count * 4;    // FIXME-OPT: Not sharing edges
            PrimReserve(vertexCount, indexCount);

            for (int i1 = 0; i1 < count; i1++)
            {
                const int i2 = (i1 + 1) == numPoints ? 0 : i1 + 1;
                const FVec2& p1 = points[i1];
                const FVec2& p2 = points[i2];

                float dx = p2.x - p1.x;
                float dy = p2.y - p1.y;
                IM_NORMALIZE2F_OVER_ZERO(dx, dy);
                dx *= (thickness * 0.5f);
                dy *= (thickness * 0.5f);

                const FVec2 uv1 = getUV(i1, p1);
                const FVec2 uv2 = getUV(i2, p2);
                vertexWritePtr[0].pos.x = p1.x + dy; vertexWritePtr[0].pos.y = p1.y - dx; vertexWritePtr[0].uv = uv1; vertexWritePtr[0].color = color; vertexWritePtr[0].drawItemIndex = drawItemIndex;
                vertexWritePtr[1].pos.x = p2.x + dy; vertexWritePtr[1].pos.y = p2.y - dx; vertexWritePtr[1].uv = uv2; vertexWritePtr[1].color = color; vertexWritePtr[1].drawItemIndex = drawItemIndex;
                vertexWritePtr[2].pos.x = p2.x - dy; vertexWritePtr[2].pos.y = p2.y + dx; vertexWritePtr[2].uv = uv2; vertexWritePtr[2].color = color; vertexWritePtr[2].drawItemIndex = drawItemIndex;
                vertexWritePtr[3].pos.x = p1.x - dy; vertexWritePtr[3].pos.y = p1.y + dx; vertexWritePtr[3].uv = uv1; vertexWritePtr[3].color = color; vertexWritePtr[3].drawItemIndex = drawItemIndex;
                vertexWritePtr += 4;

                indexWritePtr[0] = (FUIIndex)(vertexCurrentIdx); indexWritePtr[1] = (FUIIndex)(vertexCurrentIdx + 1); indexWritePtr[2] = (FUIIndex)(vertexCurrentIdx + 2);
                indexWritePtr[3] = (FUIIndex)(vertexCurrentIdx); indexWritePtr[4] = (FUIIndex)(vertexCurrentIdx + 2); indexWritePtr[5] = (FUIIndex)(vertexCurrentIdx + 3);
                indexWritePtr += 6;
                vertexCurrentIdx += 4;
            }

            drawCmd.IndexCount += indexCount;
        }
    }

    void FUIDrawList::AddConvexPolyFilled(const FVec2* points, int numPoints, u32 color, bool antiAliased, FRect* minMaxPos, u32 drawItemIndex)
    {
        ZoneScoped;

        if (points == nullptr || numPoints < 3)
            return;
        if ((color & ColorAlphaMask) == 0)
            return;

        FUIDrawCmd& drawCmd = AcquireDrawCmd();

        const FVec2 rectMin = minMaxPos ? minMaxPos->min : FVec2(0, 0);
        const FVec2 rectSize = minMaxPos ? minMaxPos->GetSize() : FVec2(1, 1);

        if (antiAliased)
        {
            const float AA_SIZE = fringeScale;
            const u32 transparentColor = color & ~ColorAlphaMask;

            const int indexCount = (numPoints - 2) * 3 + numPoints * 6;
            const int vertexCount = numPoints * 2;
            PrimReserve(vertexCount, indexCount);

            // Compute per-edge outward normals
            temporaryPoints.RemoveAll();
            temporaryPoints.InsertRange(numPoints, FVec2());
            FVec2* tempNormals = temporaryPoints.GetData();

            for (int i0 = numPoints - 1, i1 = 0; i1 < numPoints; i0 = i1++)
            {
                float dx = points[i1].x - points[i0].x;
                float dy = points[i1].y - points[i0].y;
                IM_NORMALIZE2F_OVER_ZERO(dx, dy);
                tempNormals[i0].x = dy;
                tempNormals[i0].y = -dx;
            }

            // Layout: inner[i] = vertexCurrentIdx + i*2
            //         outer[i] = vertexCurrentIdx + i*2 + 1
            const FUIIndex vtxInnerIdx = vertexCurrentIdx;
            const FUIIndex vtxOuterIdx = vertexCurrentIdx + 1;

            // Fill indices: fan from inner vertex 0
            for (int i = 2; i < numPoints; i++)
            {
                indexWritePtr[0] = (FUIIndex)(vtxInnerIdx);
                indexWritePtr[1] = (FUIIndex)(vtxInnerIdx + (i - 1) * 2);
                indexWritePtr[2] = (FUIIndex)(vtxInnerIdx + i * 2);
                indexWritePtr += 3;
            }

            // Fringe indices
            for (int i0 = numPoints - 1, i1 = 0; i1 < numPoints; i0 = i1++)
            {
                indexWritePtr[0] = (FUIIndex)(vtxInnerIdx + i1 * 2);
                indexWritePtr[1] = (FUIIndex)(vtxInnerIdx + i0 * 2);
                indexWritePtr[2] = (FUIIndex)(vtxOuterIdx + i0 * 2);
                indexWritePtr[3] = (FUIIndex)(vtxOuterIdx + i0 * 2);
                indexWritePtr[4] = (FUIIndex)(vtxOuterIdx + i1 * 2);
                indexWritePtr[5] = (FUIIndex)(vtxInnerIdx + i1 * 2);
                indexWritePtr += 6;
            }

            // Vertices: average adjacent edge normals at each point
            for (int i0 = numPoints - 1, i1 = 0; i1 < numPoints; i0 = i1++)
            {
                float dm_x = (tempNormals[i0].x + tempNormals[i1].x) * 0.5f;
                float dm_y = (tempNormals[i0].y + tempNormals[i1].y) * 0.5f;
                IM_FIXNORMAL2F(dm_x, dm_y);
                dm_x *= AA_SIZE * 0.5f;
                dm_y *= AA_SIZE * 0.5f;

                const FVec2 uv = minMaxPos ? FVec2((points[i1].x - rectMin.x) / rectSize.x,
                    (points[i1].y - rectMin.y) / rectSize.y)
                    : FVec2(0, 0);

                vertexWritePtr[0].pos = FVec2(points[i1].x - dm_x, points[i1].y - dm_y);
                vertexWritePtr[0].uv = uv; vertexWritePtr[0].color = color;
                vertexWritePtr[0].drawItemIndex = drawItemIndex;

                vertexWritePtr[1].pos = FVec2(points[i1].x + dm_x, points[i1].y + dm_y);
                vertexWritePtr[1].uv = uv; vertexWritePtr[1].color = transparentColor;
                vertexWritePtr[1].drawItemIndex = drawItemIndex;

                vertexWritePtr += 2;
            }

            vertexCurrentIdx += vertexCount;
            drawCmd.IndexCount += indexCount;
        }
        else
        {
            const int indexCount = (numPoints - 2) * 3;
            const int vertexCount = numPoints;
            PrimReserve(vertexCount, indexCount);

            for (int i = 0; i < numPoints; i++)
            {
                const FVec2 uv = minMaxPos ? FVec2((points[i].x - rectMin.x) / rectSize.x,
                    (points[i].y - rectMin.y) / rectSize.y)
                    : FVec2(0, 0);
                vertexWritePtr[i].pos = points[i];
                vertexWritePtr[i].uv = uv;
                vertexWritePtr[i].color = color;
                vertexWritePtr[i].drawItemIndex = drawItemIndex;
            }
            vertexWritePtr += numPoints;

            for (int i = 2; i < numPoints; i++)
            {
                indexWritePtr[0] = (FUIIndex)(vertexCurrentIdx);
                indexWritePtr[1] = (FUIIndex)(vertexCurrentIdx + i - 1);
                indexWritePtr[2] = (FUIIndex)(vertexCurrentIdx + i);
                indexWritePtr += 3;
            }

            vertexCurrentIdx += vertexCount;
            drawCmd.IndexCount += indexCount;
        }
    }

    void FUIDrawList::AddQuad(const FRect& rect, FVec2 uvMin, FVec2 uvMax, u32 color, u32 drawItemIndex)
    {
        ZoneScoped;

        const int indexCount = 6;
        const int vertexCount = 4;
        
        PrimReserve(vertexCount, indexCount);

        FUIDrawCmd& drawCmd = AcquireDrawCmd();

        FVec2 topLeft = rect.min;
        FVec2 topRight = FVec2(rect.max.x, rect.min.y);
        FVec2 bottomRight = FVec2(rect.max.x, rect.max.y);
        FVec2 bottomLeft = FVec2(rect.min.x, rect.max.y);

        FVec2 topLeftUV = uvMin;
        FVec2 topRightUV = FVec2(uvMax.x, uvMin.y);
        FVec2 bottomRightUV = uvMax;
        FVec2 bottomLeftUV = FVec2(uvMin.x, uvMax.y);

        vertexWritePtr[0].pos = topLeft; vertexWritePtr[0].uv = topLeftUV; vertexWritePtr[0].color = color; vertexWritePtr[0].drawItemIndex = drawItemIndex;
        vertexWritePtr[1].pos = topRight; vertexWritePtr[1].uv = topRightUV; vertexWritePtr[1].color = color; vertexWritePtr[1].drawItemIndex = drawItemIndex;
        vertexWritePtr[2].pos = bottomRight; vertexWritePtr[2].uv = bottomRightUV; vertexWritePtr[2].color = color; vertexWritePtr[2].drawItemIndex = drawItemIndex;
        vertexWritePtr[3].pos = bottomLeft; vertexWritePtr[3].uv = bottomLeftUV; vertexWritePtr[3].color = color; vertexWritePtr[3].drawItemIndex = drawItemIndex;

        indexWritePtr[0] = (FUIIndex)(vertexCurrentIdx);
        indexWritePtr[1] = (FUIIndex)(vertexCurrentIdx + 2);
        indexWritePtr[2] = (FUIIndex)(vertexCurrentIdx + 3);

        indexWritePtr[3] = (FUIIndex)(vertexCurrentIdx);
        indexWritePtr[4] = (FUIIndex)(vertexCurrentIdx + 1);
        indexWritePtr[5] = (FUIIndex)(vertexCurrentIdx + 2);

        vertexWritePtr += vertexCount;
        indexWritePtr += indexCount;

        vertexCurrentIdx += vertexCount;
        drawCmd.IndexCount += indexCount;
    }

}

#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    
    class FUSIONWIDGETS_API FPainter
    {
    public:

        FPainter(FLayer* layer);

        // - Public API -

        void ResetState();

        FLayer* GetLayer() const { return m_Layer; }

        f32 GetCurrentOpacity() const { return m_OpacityStack.IsEmpty() ? 1.0f : m_OpacityStack.Last(); }

        void SetAntiAliasing(bool enabled) { m_AntiAliased = enabled; }

        void PushTransform(const FAffineTransform& transform);
        void PopTransform();

        FAffineTransform GetCurrentTransform();

        //void SetFont(const FFont& font) { currentFont = font; }
        void SetPen(const FPen& pen) { m_CurrentPen = pen; }
        void SetBrush(const FBrush& brush) { m_CurrentBrush = brush; }

        // - Path API -

        void PathClear();

        void PathInsert(FVec2 point);
        void PathArcTo(const FVec2& center, float radius, float startAngleRadians, float endAngleRadians);
        void PathArcToFast(const FVec2& center, float radius, float startAngleRadians, float endAngleRadians);

        void PathBezierCubicCurveTo(const FVec2& p1, const FVec2& p2, const FVec2& p3, const FVec2& p4, int numSegments = 0);
        void PathQuadraticCubicCurveTo(const FVec2& p1, const FVec2& p2, const FVec2& p3, int numSegments = 0);

        void PathRect(const FRect& rect, const FVec4& cornerRadius = FVec4());

        bool PathFill();
        bool PathStroke(bool closed);
        bool PathFillAndStroke();

        // - Simple Shapes -

        void StrokeRect(const FRect& rect, const FVec4& cornerRadius = FVec4());
        void FillRect(const FRect& rect, const FVec4& cornerRadius = FVec4());
        void FillAndStrokeRect(const FRect& rect, const FVec4& cornerRadius = FVec4());

        void StrokeCircle(const FVec2& center, f32 radius);
        void FillCircle(const FVec2& center, f32 radius);
        void FillAndStrokeCircle(const FVec2& center, f32 radius);

        void DrawLine(const FVec2& p1, const FVec2& p2);

        void StrokeShape(const FRect& rect, const FShape& shape);
        void FillShape(const FRect& rect, const FShape& shape);
        void FillAndStrokeShape(const FRect& rect, const FShape& shape);

        // - Clipping -

        void PushClip(const FRect& rect, const FShape& shape);
        void PopClip();

    private:

        int CalculateNumCircleSegments(float radius) const;

        int GetCurrentClipIndex() const { return m_ClipStack.GetCount() - 1; }

        // - Path Internals -

        void PathMinMax(FVec2 point);
        void PathArcTo(const FVec2& center, float radius, float startAngleRadians, float endAngleRadians, int numSegments);
        void PathArcToFastInternal(const FVec2& center, float radius, int sampleMin, int sampleMax, int step);

        bool PathStrokeInternal(bool closed);
        bool PathFillInternal();

        using FPathArray = FStableDynamicArray<FVec2, 128>;
        using FFloatArray = FStableDynamicArray<f32, 128>;
        using FOpacityStack = FStableDynamicArray<f32, 32>;
        using FTransformStack = FStableDynamicArray<FAffineTransform, 128>;
        using FClipStack = FStableDynamicArray<int, 32>;

        static constexpr u32 kArcFastTableSize = 48;

        FLayer* m_Layer = nullptr;
        FUIDrawList* m_DrawList = nullptr;

        FPen m_CurrentPen;
        FBrush m_CurrentBrush;

        FPathArray m_Path;
        FVec2 m_PathMin, m_PathMax;

        FFloatArray m_TempPoints;

        FOpacityStack m_OpacityStack;
        FTransformStack m_TransformStack;
        FClipStack m_ClipStack;

        f32 m_DpiScale = 1.0f;

        f32 m_CircleSegmentMaxError = 0.2f;
        f32 m_CurveTessellationTolerance = 1.25f;

        FVec2 m_ArcFastVertex[kArcFastTableSize] = {};
        float m_ArcFastRadiusCutoff = 0;
        bool m_AntiAliased = true;
    };

} // namespace Fusion

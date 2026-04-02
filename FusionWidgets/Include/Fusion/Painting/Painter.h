#pragma once

namespace Fusion
{
    
    class FUSIONWIDGETS_API FPainter
    {
    public:

        FPainter(FLayer* layer);

    private:

        using FPathArray = FStableDynamicArray<FVec2, 128>;
        using FFloatArray = FStableDynamicArray<f32, 128>;
        using FOpacityStack = FStableDynamicArray<f32, 32>;
        using FTransformStack = FStableDynamicArray<FAffineTransform, 128>;
        using FClipStack = FStableDynamicArray<int, 32>;

        static constexpr u32 ArcFastTableSize = 48;

        FLayer* m_Layer = nullptr;
        FUIDrawList* drawList = nullptr;

        FPen currentPen;
        FBrush currentBrush;
        FFont currentFont;

        FPathArray path;
        FVec2 pathMin, pathMax;

        FFloatArray tempPoints;

        FOpacityStack opacityStack;
        FTransformStack transformStack;
        FClipStack clipStack;

        f32 dpiScale = 1.0f;

        f32 circleSegmentMaxError = 0.2f;
        f32 curveTessellationTolerance = 1.25f;

        FVec2 arcFastVertex[ArcFastTableSize] = {};
        float arcFastRadiusCutoff = 0;
    };

} // namespace Fusion

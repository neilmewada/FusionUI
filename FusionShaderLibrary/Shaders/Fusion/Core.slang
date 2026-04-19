
// = Data Structures =

enum class FUIShaderType : uint
{
    SolidColor = 0,
    Texture,
    LinearGradient,
    RadialGradient,
    ConicGradient,
    SDFText,
    Custom
};

enum class FUIDrawItemFlags : uint
{
    None = 0,
    TextureTileX = 1 << 0,
    TextureTileY = 1 << 1,
    ImageFitCover = 1 << 2,
    ImageFitContain = 1 << 3,
};

struct FUIDrawItem
{
    FUIShaderType shaderType;
    uint textureIndex;
    uint samplerIndex;
    FUIDrawItemFlags flags;

    int clipRectIndex;
    int gradientStartIndex;
    int gradientStopCount;
    uint userFlags;

    // 128 bytes: per-shader payload
    float data[32];
};

struct FUIClipRect
{
    float4x4 clipInverseTransform; // clip-owner layer-local → clip-local
    float4 cornerRadii;            // topLeft, topRight, bottomRight, bottomLeft
    float2 clipHalfSize;
    int _pad[2];
};

struct FUIGradientStop
{
    uint packedColor;
    float position;
};

// --------------------------------------------------------------
// = Per Application Resources =

//[[vk::binding(0, 0)]]
//SamplerState _Samplers[16];

[[vk::binding(1, 0)]]
Texture2DArray<float4> _TextureArray;

[[vk::binding(2, 0)]]
Texture2DArray<float> _FontAtlas;

[[vk::binding(3, 0)]]
SamplerState _Sampler;

// = Per Surface Resources =

[[vk::binding(0, 1)]]
cbuffer _ViewData
{
    float4x4 _ViewMatrix;
    float4x4 _ViewProjectionMatrix;
    float4x4 _ProjectionMatrix;
    float4   _ViewPosition;
    float2   _PixelResolution;
};

// -------------------------------
// = Per Layer Transforms =

[[vk::binding(0, 2)]]
cbuffer _LayerData
{
    float4x4 _LayerTransform;
};


// -------------------------------
// = Draw Items (Per Layer) =

[[vk::binding(0, 3)]]
StructuredBuffer<FUIDrawItem> _DrawItems;

[[vk::binding(1, 3)]]
StructuredBuffer<FUIClipRect> _ClipRects;

[[vk::binding(2, 3)]]
StructuredBuffer<FUIGradientStop> _GradientStops;

// ---------------------------------------------------------
// - Utility Functions -


float SDFRoundedRect(float2 p, float2 halfSize, float4 radii)
{
    float r = p.x > 0.0
                  ? (p.y > 0.0 ? radii.z : radii.y)
                  : (p.y > 0.0 ? radii.w : radii.x);
    float2 q = abs(p) - halfSize + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

inline float4 UnpackColor(uint packedColor)
{
    return float4((float)((packedColor) & 0xFF) / 255.0, (float)((packedColor >> 8) & 0xFF) / 255.0, (float)((packedColor >> 16) & 0xFF) / 255.0, (float)((packedColor >> 24) & 0xFF) / 255.0);
}

inline float2 sqr(float2 x) { return x * x; }

// ---------------------------------------------------------
// - Vertex Input

struct VSInput
{
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    uint drawItemIndex : TEXCOORD1;
};


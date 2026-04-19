
[[vk::binding(0, 0)]]
Texture2D<float4> _InputTexture;

[[vk::binding(1, 0)]]
SamplerState _InputTextureSampler;

struct VSInput
{
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
}

[shader("vertex")]
VSOutput VSMain(VSInput input)
{
    VSOutput o;
    o.position = float4(input.position, 0.0, 1.0);
    o.uv = input.uv;
    return o;
}

[shader("fragment")]
float4 PSMain(VSOutput input) : SV_Target
{
    const float weight = 1.0 / 9.0;

    uint width;
    uint height;
    _InputTexture.GetDimensions(width, height);

    float2 texCoord = input.uv;
    float2 samplePos = texCoord * float2(width, height);

    float4 colorSum = float4(0.0, 0.0, 0.0, 0.0);

    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float4 color = _InputTexture.Sample(_InputTextureSampler, (samplePos + float2(x, y)) / float2(width, height));

            colorSum += color * weight;
        }
    }

    return colorSum;
}

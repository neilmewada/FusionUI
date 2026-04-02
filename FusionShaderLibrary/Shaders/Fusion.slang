import Fusion.Core;

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    nointerpolation uint drawItemIndex : TEXCOORD1;
    float2 layerPos : TEXCOORD2;
};

[shader("vertex")]
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = mul(mul(float4(input.position, 0.0, 1.0), _LayerTransform), _ViewProjectionMatrix);
    output.uv = input.uv;
    output.color = input.color;
    output.drawItemIndex = input.drawItemIndex;
    output.layerPos = input.position;
    return output;
}

[shader("fragment")]
float4 PSMain(VSOutput input) : SV_Target
{
    return input.color;
}

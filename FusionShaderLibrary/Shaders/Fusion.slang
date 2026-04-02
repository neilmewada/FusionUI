import Fusion.Core;

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    nointerpolation uint drawItemIndex : TEXCOORD1;
};

[shader("vertex")]
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    output.drawItemIndex = input.drawItemIndex;
    return output;
}

[shader("fragment")]
float4 PSMain(VSOutput input) : SV_Target
{
    return input.color;
}

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
    FUIDrawItem item = _DrawItems[input.drawItemIndex];
    float4 color = input.color;
    float2 uv = input.uv;
    const FUIDrawItemFlags flags = item.flags;

    // - Clipping -

    if (item.clipRectIndex >= 0)
    {
        FUIClipRect clip = _ClipRects[item.clipRectIndex];
        float2 clipLocalPos = mul(float4(input.layerPos, 0.0, 1.0), clip.clipInverseTransform).xy;
        float dist = SDFRoundedRect(clipLocalPos, clip.clipHalfSize, clip.cornerRadii);
        color.a *= saturate(0.5 - dist);
    }

    // - Shading -

    if (item.shaderType == FUIShaderType::Texture)
    {
        float2 sampleUVMin = float2(item.data[0], item.data[1]);
        float2 sampleUVMax = float2(item.data[2], item.data[3]);
        float2 fitOffset = float2(item.data[4], item.data[5]);
        float2 fitSize = float2(item.data[6], item.data[7]);

        const bool contain = flags & FUIDrawItemFlags::ImageFitContain;
        const bool cover = flags & FUIDrawItemFlags::ImageFitCover;
        const bool tileX = flags & FUIDrawItemFlags::TextureTileX;
        const bool tileY = flags & FUIDrawItemFlags::TextureTileY;
        const bool noTile = !tileX && !tileY;

        float2 imgUV = (input.uv - fitOffset) / fitSize;
        if (tileX) imgUV.x = frac(imgUV.x);
        if (tileY) imgUV.y = frac(imgUV.y);

        float2 sampleUV = lerp(sampleUVMin, sampleUVMax, imgUV);
        float4 sampleColor;

        if ((!tileX && (imgUV.x < 0 || imgUV.x > 1)) || (!tileY && (imgUV.y < 0 || imgUV.y > 1)))
            sampleColor = float4(0, 0, 0, 0);
        else
            sampleColor = _TextureArray.Sample(_FontAtlasSampler, float3(sampleUV, item.textureIndex));

        color *= sampleColor;
    }
    else if (item.shaderType == FUIShaderType::LinearGradient)
    {
        float angle = item.data[0];
        uv = fmod(uv, 1.00001f);

        float2 dir = float2(cos(angle), sin(angle));
        float range = abs(dir.x) + abs(dir.y);
        float gradientT = dot(uv - 0.5, dir) / range + 0.5;

        for (int i = 0; i < item.gradientStopCount - 1; i++)
        {
            int idx = item.gradientStartIndex + i;

            const FUIGradientStop left = _GradientStops[idx];
            const FUIGradientStop right = _GradientStops[idx + 1];

            if (left.position <= gradientT && gradientT < right.position)
            {
                float segT = clamp((gradientT - left.position) / (right.position - left.position), 0.0, 1.0);
                float4 sampleColor = lerp(UnpackColor(left.packedColor), UnpackColor(right.packedColor), segT);
                color *= sampleColor;
                break;
            }
        }
    }
    else if (item.shaderType == FUIShaderType::SDFText)
    {
        float pxRange = item.data[0];
        float atlasSize = item.data[1];

        // How many atlas texels correspond to one unit of SDF distance
        float2 unitRange = float2(pxRange / atlasSize, pxRange / atlasSize);

        float2 fw = fwidth(uv);

        // How many screen pixels correspond to one atlas texel at current scale
        // float2 screenTexSize = float2(1.0) / fw;
        float2 screenTexSize = rsqrt(sqr(ddx(uv)) + sqr(ddy(uv)));

        // Effective screen-pixel range of the SDF transition band
        float screenPxRange = max(0.5 * dot(unitRange, screenTexSize), 1.0);

        float sd = _FontAtlas.Sample(_FontAtlasSampler, float3(uv, float(item.textureIndex)));

        float screenPxDistance = screenPxRange * (sd - 0.5);
        float alpha = clamp(screenPxDistance + 0.5, 0.0, 1.0);

        color.a *= alpha;
    }

    return color;
}

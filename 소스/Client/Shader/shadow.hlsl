#include "common.hlsl"

struct GS_INPUT
{
    float4 positionW : SV_POSITION;
};

struct GS_OUTPUT
{
    float4 positionH : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

GS_INPUT VS_ANIMATION(VS_INPUT input)
{
    GS_INPUT output;
    float4 posL = float4(0.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < 4; ++i)
    {
        posL += input.boneWeight[i] * mul(input.position, g_boneTransformMatrix[input.boneIndex[i]]);
    }
    output.positionW = mul(mul(posL, g_meshTransformMatrix), g_worldMatrix);
    return output;
}

GS_INPUT VS_MODEL(VS_INPUT input)
{
    GS_INPUT output;
    output.positionW = mul(mul(input.position, g_meshTransformMatrix), g_worldMatrix);
    return output;
}

GS_INPUT VS_LINK(VS_INPUT input)
{
    GS_INPUT output;
    output.positionW = mul(input.position, g_meshTransformMatrix);
    output.positionW = mul(output.positionW, g_boneTransformMatrix[input.boneIndex[0]]);
    output.positionW = mul(output.positionW, g_worldMatrix);
    return output;
}

[maxvertexcount(3 * SHADOWMAP_COUNT)]
void GS(triangle GS_INPUT input[3], inout TriangleStream<GS_OUTPUT> output)
{
    for (int i = 0; i < SHADOWMAP_COUNT; ++i)
    {
        GS_OUTPUT result;
        result.RTIndex = i;
        for (int j = 0; j < 3; ++j)
        {
            result.positionH = mul(mul(input[j].positionW, g_shadowLight.lightViewMatrix[i]), g_shadowLight.lightProjMatrix[i]);
            output.Append(result);
        }
        output.RestartStrip();
    }
}
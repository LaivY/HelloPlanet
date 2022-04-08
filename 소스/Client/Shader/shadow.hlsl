#include "common.hlsl"

PS_INPUT VS_MODEL(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(mul(input.position, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_lights[0].lightViewMatrix), g_lights[0].lightProjMatrix);
    return output;
}

PS_INPUT VS_LINK(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, g_meshTransformMatrix);
    output.positionW = mul(output.positionW, g_boneTransformMatrix[input.boneIndex[0]]);
    output.positionW = mul(output.positionW, g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_lights[0].lightViewMatrix), g_lights[0].lightProjMatrix);
    return output;
}

PS_INPUT VS_ANIMATION(VS_INPUT input)
{
    PS_INPUT output;
    float4 posL = float4(0.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < 4; ++i)
    {
        posL += input.boneWeight[i] * mul(input.position, g_boneTransformMatrix[input.boneIndex[i]]);
    }
    output.positionW = mul(mul(posL, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_lights[0].lightViewMatrix), g_lights[0].lightProjMatrix);
    return output;
}
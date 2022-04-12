#include "common.hlsl"

PS_INPUT VS_MODEL_S(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(mul(input.position, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_shadowLight.lightViewMatrix[0]), g_shadowLight.lightProjMatrix[0]);
    return output;
}

PS_INPUT VS_MODEL_M(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(mul(input.position, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_shadowLight.lightViewMatrix[1]), g_shadowLight.lightProjMatrix[1]);
    return output;
}

PS_INPUT VS_MODEL_L(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(mul(input.position, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_shadowLight.lightViewMatrix[2]), g_shadowLight.lightProjMatrix[2]);
    return output;
}

PS_INPUT VS_MODEL_ALL(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(mul(input.position, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_shadowLight.lightViewMatrix[3]), g_shadowLight.lightProjMatrix[3]);
    return output;
}

PS_INPUT VS_LINK_S(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, g_meshTransformMatrix);
    output.positionW = mul(output.positionW, g_boneTransformMatrix[input.boneIndex[0]]);
    output.positionW = mul(output.positionW, g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_shadowLight.lightViewMatrix[0]), g_shadowLight.lightProjMatrix[0]);
    return output;
}

PS_INPUT VS_LINK_M(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, g_meshTransformMatrix);
    output.positionW = mul(output.positionW, g_boneTransformMatrix[input.boneIndex[0]]);
    output.positionW = mul(output.positionW, g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_shadowLight.lightViewMatrix[1]), g_shadowLight.lightProjMatrix[1]);
    return output;
}

PS_INPUT VS_LINK_L(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, g_meshTransformMatrix);
    output.positionW = mul(output.positionW, g_boneTransformMatrix[input.boneIndex[0]]);
    output.positionW = mul(output.positionW, g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_shadowLight.lightViewMatrix[2]), g_shadowLight.lightProjMatrix[2]);
    return output;
}

PS_INPUT VS_LINK_ALL(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, g_meshTransformMatrix);
    output.positionW = mul(output.positionW, g_boneTransformMatrix[input.boneIndex[0]]);
    output.positionW = mul(output.positionW, g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_shadowLight.lightViewMatrix[3]), g_shadowLight.lightProjMatrix[3]);
    return output;
}

PS_INPUT VS_ANIMATION_S(VS_INPUT input)
{
    PS_INPUT output;
    float4 posL = float4(0.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < 4; ++i)
    {
        posL += input.boneWeight[i] * mul(input.position, g_boneTransformMatrix[input.boneIndex[i]]);
    }
    output.positionW = mul(mul(posL, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_shadowLight.lightViewMatrix[0]), g_shadowLight.lightProjMatrix[0]);
    return output;
}

PS_INPUT VS_ANIMATION_M(VS_INPUT input)
{
    PS_INPUT output;
    float4 posL = float4(0.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < 4; ++i)
    {
        posL += input.boneWeight[i] * mul(input.position, g_boneTransformMatrix[input.boneIndex[i]]);
    }
    output.positionW = mul(mul(posL, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_shadowLight.lightViewMatrix[1]), g_shadowLight.lightProjMatrix[1]);
    return output;
}

PS_INPUT VS_ANIMATION_L(VS_INPUT input)
{
    PS_INPUT output;
    float4 posL = float4(0.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < 4; ++i)
    {
        posL += input.boneWeight[i] * mul(input.position, g_boneTransformMatrix[input.boneIndex[i]]);
    }
    output.positionW = mul(mul(posL, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_shadowLight.lightViewMatrix[2]), g_shadowLight.lightProjMatrix[2]);
    return output;
}

PS_INPUT VS_ANIMATION_ALL(VS_INPUT input)
{
    PS_INPUT output;
    float4 posL = float4(0.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < 4; ++i)
    {
        posL += input.boneWeight[i] * mul(input.position, g_boneTransformMatrix[input.boneIndex[i]]);
    }
    output.positionW = mul(mul(posL, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_shadowLight.lightViewMatrix[3]), g_shadowLight.lightProjMatrix[3]);
    return output;
}
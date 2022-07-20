#include "common.hlsl"

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    float4 posL = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float3 normalL = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        posL += input.boneWeight[i] * mul(input.position, g_boneTransformMatrix[input.boneIndex[i]]);
        normalL += input.boneWeight[i] * mul(input.normal, (float3x3) g_boneTransformMatrix[input.boneIndex[i]]);
    }
    output.positionW = mul(mul(posL, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_viewMatrix), g_projMatrix);
    output.normalW = mul(normalL, (float3x3) g_meshTransformMatrix);
    output.normalW = mul(output.normalW, (float3x3) g_worldMatrix);
    output.uv = input.uv;
    output.materialIndex = input.materialIndex;
    return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    float4 output = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (input.materialIndex < 0)
    {
        output = g_texture.Sample(g_sampler, input.uv);
    }
    else
    {
        output = g_materials[input.materialIndex].baseColor;
    }
    float4 lightColor = Lighting(input.positionW.xyz, input.normalW, input.materialIndex);
    float shadowFactor = CalcShadowFactor(input.positionW);

    if (shadowFactor > 0.66f)
        shadowFactor = 0.66f;
    else if (shadowFactor > 0.33f)
        shadowFactor = 0.33f;
    else
        shadowFactor = 0.0f;

    return output + lightColor * shadowFactor;
}
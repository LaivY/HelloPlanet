#include "common.hlsl"

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_viewMatrix), g_projMatrix);
    output.normalW = mul(input.normal, (float3x3) g_worldMatrix);
    output.uv = input.uv;
    output.materialIndex = input.materialIndex;
    return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    float4 output = 0;
    if (input.materialIndex < 0)
    {
        output = g_texture.Sample(g_sampler, input.uv);
    }
    else
    {
        output = g_materials[input.materialIndex].baseColor;
    }

    float4 gradiant = float4(0.1f, 0.1f, 0.1f, 1.0f);
    float l = length(g_eye - input.positionW.xyz);
    l /= 7000.0f;
    gradiant.xyz += l;

    float shadowFactor = CalcShadowFactor(input.positionW);
    return output + gradiant * shadowFactor;
}
#include "common.hlsl"

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(mul(input.position, g_meshTransformMatrix), g_worldMatrix);
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

    float4 lightColor = 0;
    float intensity = dot(g_shadowLight.direction, -normalize(input.normalW));
    if (intensity > 0.95f)
        lightColor += float4(g_shadowLight.color, 1.0f) * 1.0f;
    else if (intensity > 0.5f)
        lightColor += float4(g_shadowLight.color, 1.0f) * 0.7f;
    else if (intensity > 0.05f)
        lightColor += float4(g_shadowLight.color, 1.0f) * 0.35f;
    else
        lightColor += float4(g_shadowLight.color, 1.0f) * 0.1f;

    float shadowFactor = CalcShadowFactor(input.positionW);
    if (shadowFactor < 0.33f)
        shadowFactor = 0.33f;
    else if (shadowFactor < 0.66f)
        shadowFactor = 0.66f;
    else
        shadowFactor = 0.99f;
    return output + lightColor * shadowFactor;
}

float4 PSSkybox(PS_INPUT input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}
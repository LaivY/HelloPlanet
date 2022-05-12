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
    if (intensity > 0.95)
        lightColor += float4(g_shadowLight.color, 1.0f) * 1.0f;
    else if (intensity > 0.5)
        lightColor += float4(g_shadowLight.color, 1.0f) * 0.7f;
    else if (intensity > 0.05)
        lightColor += float4(g_shadowLight.color, 1.0f) * 0.35f;
    else
        lightColor += float4(g_shadowLight.color, 1.0f) * 0.1f;

    float shadowFactor = CalcShadowFactor(input.positionW);
    return output + lightColor * shadowFactor;
}

PS_INPUT VS_OUTLINE(VS_INPUT input)
{
    PS_INPUT output;

    float4 position = float4(input.position.xyz + input.normal * 1.5f, 1.0f);

    output.positionW = mul(mul(position, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_viewMatrix), g_projMatrix);
    output.normalW = mul(input.normal, (float3x3) g_worldMatrix);
    output.uv = input.uv;
    output.materialIndex = input.materialIndex;
    return output;
}

float4 PS_OUTLINE(PS_INPUT input) : SV_TARGET
{
    return float4(0.1f, 0.1f, 0.1f, 1.0f);
}

float4 PSSkybox(PS_INPUT input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}
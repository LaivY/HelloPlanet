#include "common.hlsl"
#define LIGHTING

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(mul(input.position, g_meshTransformMatrix), g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_viewMatrix), g_projMatrix);
    output.shadowPosH = mul(mul(mul(output.positionW, g_lights[0].lightViewMatrix), g_lights[0].lightProjMatrix), toTextureMatrix);
    output.normalW = mul(input.normal, (float3x3) g_worldMatrix);
    output.color = input.color;
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
    float shadowFactor = CalcShadowFactor(input.shadowPosH);
    return output + lightColor * shadowFactor;
}

float4 PSSkybox(PS_INPUT input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}
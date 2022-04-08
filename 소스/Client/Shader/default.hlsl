#include "common.hlsl"
#define SHADOW

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_viewMatrix), g_projMatrix);
    output.shadowPosH = mul(mul(output.positionW, g_lights[0].lightViewMatrix), g_lights[0].lightProjMatrix);
    output.normalW = mul(input.normal, (float3x3) g_worldMatrix);
    output.color = input.color;
    output.uv = input.uv;
    output.materialIndex = input.materialIndex;
    return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    float4 lightColor = Lighting(input.positionW.xyz, input.normalW, input.materialIndex);
    float shadowFactor = 1.0f;
#ifdef SHADOW
    shadowFactor = CalcShadowFactor(input.shadowPosH);
#endif
    return g_materials[input.materialIndex].baseColor + lightColor * shadowFactor;
}
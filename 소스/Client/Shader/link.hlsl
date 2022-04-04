#include "common.hlsl"
#define LIGHTING

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, g_meshTransformMatrix);
    output.positionW = mul(output.positionW, g_boneTransformMatrix[input.boneIndex[0]]);
    output.positionW = mul(output.positionW, g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_viewMatrix), g_projMatrix);
    //output.shadowPosH = mul(mul(mul(output.positionW, g_lightViewMatrix), g_lightProjMatrix), g_NDCToTextureMatrix);
    output.normalW = mul(input.normal, (float3x3) g_worldMatrix);
    output.color = input.color;
    output.uv = input.uv;
    output.materialIndex = input.materialIndex;
    return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
#ifdef LIGHTING
    float4 lightColor = Lighting(input.positionW.xyz, input.normalW, input.materialIndex);
    return g_materials[input.materialIndex].baseColor + lightColor;
#else
    return g_materials[input.materialIndex].baseColor;
#endif
}
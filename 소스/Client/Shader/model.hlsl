#include "common.hlsl"
//#define LIGHTING

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(mul(input.position, g_meshTransformMatrix), g_worldMatrix);
    //output.positionW = mul(output.positionW, g_worldMatrix);
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
    float4 lightColor = Lighting(input.positionW, input.normalW);
    return g_materials[input.materialIndex].baseColor + lightColor;
#else
    if (input.materialIndex < 0)
    {
        return g_texture.Sample(g_sampler, input.uv);
        //return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    return g_materials[input.materialIndex].baseColor;
#endif
}
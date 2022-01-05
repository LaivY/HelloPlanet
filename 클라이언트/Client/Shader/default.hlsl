#include "common.hlsl"

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, worldMatrix);
    output.positionH = mul(mul(output.positionW, viewMatrix), projMatrix);
    output.shadowPosH = mul(mul(mul(output.positionW, lightViewMatrix), lightProjMatrix), NDCToTextureMatrix);
    output.normalW = mul(input.normal, (float3x3)worldMatrix);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    float4 output = input.color;
    output += Lighting(materials[0], input.positionW.xyz, input.normalW, false, input.shadowPosH);
    return output;
}
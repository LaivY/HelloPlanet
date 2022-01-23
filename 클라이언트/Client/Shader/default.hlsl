#include "common.hlsl"
#define ANIMATION
//#define LIGHTING

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output;

#ifdef ANIMATION
	float4 posL = float4(0.0f, 0.0f, 0.0f, 1.0f);
	for (int i = 0; i < 4; ++i)
	{
		posL += input.boneWeight[i] * mul(input.position, g_boneTransformMatrix[input.boneIndex[i]]);
	}
	output.positionW = mul(posL, g_worldMatrix);
#else
	output.positionW = mul(input.position, g_worldMatrix);
#endif

	output.positionH = mul(mul(output.positionW, g_viewMatrix), g_projMatrix);
	output.shadowPosH = mul(mul(mul(output.positionW, g_lightViewMatrix), g_lightProjMatrix), g_NDCToTextureMatrix);
	output.normalW = mul(input.normal, (float3x3)g_worldMatrix);
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
    return g_materials[input.materialIndex].baseColor;
#endif
}
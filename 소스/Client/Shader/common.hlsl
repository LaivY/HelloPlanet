#include "lighting.hlsl"
#define MAX_MATERIAL	10
#define MAX_JOINT       96

Texture2D g_texture                     : register(t0);
Texture2D g_shadowMap                   : register(t1);
SamplerState g_sampler                  : register(s0);
SamplerComparisonState g_shadowSampler  : register(s1);

cbuffer cbGameObject : register(b0)
{
	matrix g_worldMatrix;
};

cbuffer cbMesh : register(b1)
{
    matrix g_meshTransformMatrix;
    Material g_materials[MAX_MATERIAL];
    matrix g_boneTransformMatrix[MAX_JOINT];
};

cbuffer cbCamera : register(b2)
{
	matrix g_viewMatrix;
	matrix g_projMatrix;
	float3 g_eye;
}

cbuffer cbScene : register(b3)
{
    Light g_lights[MAX_LIGHT];
}

cbuffer cbGameFramework : register(b4)
{
	float g_deltaTime;
}

struct VS_INPUT
{
	float4 position     : POSITION;
	float3 normal       : NORMAL;
	float4 color        : COLOR;
	float2 uv           : TEXCOORD;
	int materialIndex   : MATERIAL;
	uint4 boneIndex     : BONEINDEX;
	float4 boneWeight   : BONEWEIGHT;
};

struct PS_INPUT
{
	float4 positionH    : SV_POSITION;
	float4 positionW    : POSITION0;
	float4 shadowPosH   : POSITION1;
	float3 normalW      : NORMAL;
	float4 color        : COLOR;
	float2 uv           : TEXCOORD;
	int materialIndex   : MATERIAL;
};

float4 Lighting(float3 position, float3 normal, int materialIndex)
{
    float3 lightColor = float3(0.0f, 0.0f, 0.0f);

	normal = normalize(normal);
	float3 toEye = normalize(g_eye - position);

	Material material;
	if (materialIndex < 0)
	{
		// 재질이 없다면 기본 재질로 설정
		material.baseColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
		material.reflection = float3(0.1f, 0.1f, 0.1f);
		material.roughness = 0.95f;
	}
	else
	{
		material = g_materials[materialIndex];
	}

    for (int i = 0; i < MAX_LIGHT; ++i)
    {
		lightColor += ComputeDirectionalLight(g_lights[i], material, normal, toEye);
    }
    return float4(lightColor, material.baseColor.a);
}
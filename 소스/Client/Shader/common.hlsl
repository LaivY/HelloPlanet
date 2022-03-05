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

float4 Lighting(float4 position, float3 normal)
{
    float4 lightColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < MAX_LIGHT; ++i)
    {
        float4 diffuse = dot(-g_lights[i].direction, normal);
        diffuse = ceil(diffuse * 5) / 5.0f;
        lightColor += diffuse;
    }
    return saturate(lightColor);
}
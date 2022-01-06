#include "lighting.hlsl"
#define MAX_LIGHT       1
#define MAX_MATERIAL    2
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
    Material g_materials[MAX_MATERIAL];
    
    matrix g_lightViewMatrix;
    matrix g_lightProjMatrix;
    matrix g_NDCToTextureMatrix;
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
};

struct PatchTess
{
    float EdgeTess[4]   : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

// --------------------------------------

float CalcShadowFactor(float4 shadowPosH)
{   
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;

    uint width, height, numMips;
    g_shadowMap.GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float)width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += g_shadowMap.SampleCmpLevelZero(g_shadowSampler, shadowPosH.xy + offsets[i], depth).r;
    }
    return percentLit / 9.0f;
}

float4 Lighting(Material material, float3 positionW, float3 normalW, bool shadowing, float4 shadowPosH)
{
    float3 output = float3(0.0f, 0.0f, 0.0f);
    
    // 노말, 점에서 카메라를 바라보는 방향 벡터
    normalW = normalize(normalW);
    float3 toEye = normalize(g_eye - positionW);
    
    for (int i = 0; i < MAX_LIGHT; ++i)
    {
        // 꺼져있는 조명은 패스
        if (!g_lights[i].isActivate)
            continue;
        
        // 그림자 계수 계산
        float shadowFactor = 1.0f;
        if (shadowing)
        {
            shadowFactor = CalcShadowFactor(shadowPosH);
        }
        
        // 조명 계산
        if (g_lights[i].type == DIRECTIONAL_LIGHT)
        {
            output += ComputeDirectionalLight(g_lights[i], material, normalW, toEye) * shadowFactor;
        }
    }
    return float4(output, material.diffuseAlbedo.a);
}
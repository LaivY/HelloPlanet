#include "common.hlsl"

struct GS_INPUT
{
    float3 position     : POSITION;
    float3 direction    : DIRECTION;
    float speed         : SPEED;
    float lifeTime      : LIFETIME;
    float age           : AGE;
};

struct GS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

GS_INPUT VS(GS_INPUT input)
{
    return input;
}

[maxvertexcount(1)]
void GS_STREAM(point GS_INPUT input[1], inout PointStream<GS_INPUT> output)
{
    GS_INPUT particle = input[0];

    float age = frac(particle.age / particle.lifeTime) * particle.lifeTime;
    particle.position = particle.direction * particle.speed * age;
    particle.age += g_deltaTime;

    output.Append(particle);
}

[maxvertexcount(4)]
void GS(point GS_INPUT input[1], inout TriangleStream<GS_OUTPUT> outputStream)
{
    float3 positionW = mul(float4(input[0].position, 1.0f), g_worldMatrix);

    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = normalize(g_eye - positionW);
    float3 right = normalize(cross(up, look));
    up = normalize(cross(look, right));
    
    static const float PARTICLE_LENGTH = 0.1f;
    float hw = 0.5f * PARTICLE_LENGTH;
    float hh = 0.5f * PARTICLE_LENGTH;
    
    float4 position[4] =
    {
        float4(positionW + (hw * right) - (hh * up), 1.0f), // LB
        float4(positionW + (hw * right) + (hh * up), 1.0f), // LT
        float4(positionW - (hw * right) - (hh * up), 1.0f), // RB
        float4(positionW - (hw * right) + (hh * up), 1.0f)  // RT
    };
    
    float2 uv[4] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    GS_OUTPUT output = (GS_OUTPUT)0;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        output.position = mul(position[i], g_viewMatrix);
        output.position = mul(output.position, g_projMatrix);
        output.uv = uv[i];
        outputStream.Append(output);
    }
}

float4 PS(GS_OUTPUT input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
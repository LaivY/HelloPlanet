#include "common.hlsl"

static const float PI = 3.141592f;

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
    float lifeTime  : LIFETIME;
    float age       : AGE;
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
    particle.position.x -= 4.0f * sin(age * PI);
    particle.position.y -= particle.speed * age;

    particle.age += g_deltaTime;
    age = frac(particle.age / particle.lifeTime) * particle.lifeTime;
    particle.position.x += 4.0f * sin(age * PI);
    particle.position.y += particle.speed * age;

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
    
    float age = frac(input[0].age / input[0].lifeTime); // 0 ~ 1
    float value = sin(age * PI);                        // 0 ~ 1
    float PARTICLE_LENGTH = 1.0f * value;
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
        output.lifeTime = input[0].lifeTime;
        output.age = input[0].age;
        outputStream.Append(output);
    }
}

float4 PS(GS_OUTPUT input) : SV_TARGET
{
    float dist = distance(input.uv, float2(0.5f, 0.5f));
    if (dist < 0.5f) {
        // ???????????? ????????? ?????? ????????????
        float alpha = lerp(1.0f, 0.0f, dist * 2.0f);

        // ????????? ????????? ?????? ??????, ????????? ?????? ????????????
        float age = frac(input.age / input.lifeTime);
        alpha *= 0.5f - 0.5f * cos(age * 2.0f * PI);

        return float4(1.0f, 1.0f, 1.0f, alpha);
    }
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}
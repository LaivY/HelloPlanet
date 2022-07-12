#include "common.hlsl"

PS_INPUT VS(VS_INPUT input)
{
    matrix worldMatrix = g_worldMatrix;
    worldMatrix[3][3] = 1.0f;

    PS_INPUT output;
    output.positionW = mul(input.position, worldMatrix);
    output.positionH = mul(mul(output.positionW, g_viewMatrix), g_projMatrix);
    output.uv = input.uv;
    return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}

float4 PS_SKILL_GAGE(PS_INPUT input) : SV_TARGET
{
    // 0~255 RGB
    float4 baseColor = float4(0.0f, 0.0f, 0.0f, 0.2f);
    float4 activeColor = float4(0x15, 0xf2, 0xfd, 0.8f);
    baseColor.xyz /= 255.0f;
    activeColor.xyz /= 255.0f;

    float angle = g_worldMatrix[3][3];
    float dist = distance(input.uv, float2(0.5f, 0.5f));

    // 게이지
    if (0.35f < dist && dist < 0.5f)
    {
        float2 v1 = float2(0.0f, -1.0f);
        float2 v2 = normalize(input.uv - float2(0.5f, 0.5f));
        float theta = degrees(acos(dot(v1, v2)));
        if (v2.x < 0.0f)
            theta = 360.0f - theta;

        if (0 <= theta % 18 && theta % 18 <= 3)
            return float4(0.0f, 0.0f, 0.0f, 0.0f);
        if (theta <= angle)
            return activeColor;
        else
            return baseColor;
    }

    // 안쪽
    else if (0.3f < dist && dist < 0.32f)
    {
        float2 v1 = float2(0.0f, -1.0f);
        float2 v2 = normalize(input.uv - float2(0.5f, 0.5f));
        float theta = degrees(acos(dot(v1, v2)));

        if (13 <= theta % 30 && theta % 30 <= 17)
            return float4(0.0f, 0.0f, 0.0f, 0.0f);
        else
            return activeColor;
    }
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

float4 PS_WARNING(PS_INPUT input) : SV_TARGET
{
    float4 color = g_texture.Sample(g_sampler, input.uv);
    color.a *= g_worldMatrix[3][3]; // 여기에 알파값을 저장해둠
    return color;
}
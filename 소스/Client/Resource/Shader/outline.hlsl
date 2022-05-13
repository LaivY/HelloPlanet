#include "common.hlsl"

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.positionH = input.position;
    output.uv = input.uv;
    return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    float width, height;
    g_stencil.GetDimensions(width, height);

    const float4 outlineColor = float4(0.1f, 0.1f, 0.1f, 1.0f);
    const float outlineThickness = 1.0f;
    const float outlineThreshHold = 1.0f;

    // 정수 텍스쳐에서는 샘플링 할 수 없다.
    // 따라서 uv좌표를 실제 픽셀의 좌표로 설정해야한다.
    float2 uv;
    uv.x = lerp(0.0f, width, input.uv.x);
    uv.y = lerp(0.0f, height, input.uv.y);

    float2 tx = float2(outlineThickness, 0.0);
    float2 ty = float2(0.0, outlineThickness);

    float s00 = g_stencil[uv - tx - ty].g;
    float s01 = g_stencil[uv      - ty].g;
    float s02 = g_stencil[uv + tx - ty].g;
    float s10 = g_stencil[uv - tx     ].g;
    float s12 = g_stencil[uv + tx     ].g;
    float s20 = g_stencil[uv - tx + ty].g;
    float s21 = g_stencil[uv      + ty].g;
    float s22 = g_stencil[uv + tx + ty].g;

    float sx = s00 + 2.0f * s10 + s20 - s02 - 2.0f * s12 - s22;
    float sy = s00 + 2.0f * s01 + s02 - s20 - 2.0f * s21 - s22;
    float dist = sqrt(sx * sx + sy * sy);
    float edge = dist > outlineThreshHold ? 1.0f : 0.0f;
    return float4(outlineColor.rgb, edge);
}
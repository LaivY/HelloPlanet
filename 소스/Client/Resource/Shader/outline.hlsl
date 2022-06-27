#include "common.hlsl"

static const float xFilter[9] = 
{
    -1.0f, 0.0f, 1.0f,
    -2.0f, 0.0f, 2.0f,
    -1.0f, 0.0f, 1.0f
};

static const float yFilter[9] =
{
     1.0f,  2.0f,  1.0f,
     0.0f,  0.0f,  0.0f,
    -1.0f, -2.0f, -1.0f
};

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

    // 색깔과 두께는 외곽선 오브젝트의 월드 변환 행렬에 설정해둠
    const float3 outlineColor = g_worldMatrix[3].rgb;
    const float outlineThickness = g_worldMatrix[3].a;
    const float outlineThreshHold = 1.0f;

    // 정수 텍스쳐에서는 샘플링 할 수 없다.
    // 따라서 uv좌표를 실제 픽셀의 좌표로 설정해야한다.
    float2 uv;
    uv.x = lerp(0.0f, width, input.uv.x);
    uv.y = lerp(0.0f, height, input.uv.y);

    float2 tx = float2(outlineThickness, 0.0);
    float2 ty = float2(0.0, outlineThickness);

    float values[9];
    values[0] = g_stencil[uv - tx - ty].g;
    values[1] = g_stencil[uv      - ty].g;
    values[2] = g_stencil[uv + tx - ty].g;
    values[3] = g_stencil[uv - tx     ].g;
    values[4] = g_stencil[uv          ].g;
    values[5] = g_stencil[uv + tx     ].g;
    values[6] = g_stencil[uv - tx + ty].g;
    values[7] = g_stencil[uv      + ty].g;
    values[8] = g_stencil[uv + tx + ty].g;
    if (values[4] == 0)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);

    tx /= width;
    ty /= height;
    float depths[9];
    depths[0] = g_depth.Sample(g_sampler, input.uv - tx - ty);
    depths[1] = g_depth.Sample(g_sampler, input.uv      - ty);
    depths[2] = g_depth.Sample(g_sampler, input.uv + tx - ty);
    depths[3] = g_depth.Sample(g_sampler, input.uv - tx     );
    depths[4] = g_depth.Sample(g_sampler, input.uv          );
    depths[5] = g_depth.Sample(g_sampler, input.uv + tx     );
    depths[6] = g_depth.Sample(g_sampler, input.uv - tx + ty);
    depths[7] = g_depth.Sample(g_sampler, input.uv      + ty);
    depths[8] = g_depth.Sample(g_sampler, input.uv + tx + ty);

    // 현재 픽셀의 스텐실 값과 다른 픽셀의 깊이 값이 
    // 현재 픽셀의 깊이 값보다 더 작다면(가까이 있다면)
    // 해당 픽셀의 스텐실 값을 현재 픽셀의 스텐실 값과 같은 것으로 취급함
    // +
    // 현재 픽셀의 스텐실 값과 다른 값들은 다 0으로 취급함
    for (int i = 0; i < 9; ++i)
    {
        if (values[i] == values[4]) continue;
        if (depths[i] < depths[4])
        {
            return float4(0.0f, 0.0f, 0.0f, 0.0f);
        }
        values[i] = 0.0f;
    }

    // 현재 픽셀의 스텐실 값은 1로 취급함
    values[4] = 1.0f;

    float x = 0.0f, y = 0.0f;
    for (int i = 0; i < 9; ++i)
    {
        x += values[i] * xFilter[i];
        y += values[i] * yFilter[i];
    }

    float dist = sqrt(x * x + y * y);
    float edge = dist > outlineThreshHold ? 1.0f : 0.0f;
    return float4(outlineColor.rgb, edge);
}
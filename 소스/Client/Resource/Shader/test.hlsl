Texture2D<uint2> g_input        : register(t0);
RWTexture2D<float4> g_output    : register(u0);

[numthreads(32, 32, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const float4 outlineColor = float4(0.1f, 0.1f, 0.1f, 1.0f);
    const float outlineThickness = 1.0f;
    const float outlineThreshHold = 0.0f;

    float2 tx = float2(outlineThickness, 0.0);
    float2 ty = float2(0.0, outlineThickness);

    float s00 = g_input[dispatchThreadID.xy - tx - ty].g;
    float s01 = g_input[dispatchThreadID.xy      - ty].g;
    float s02 = g_input[dispatchThreadID.xy + tx - ty].g;
    float s10 = g_input[dispatchThreadID.xy - tx     ].g;
    float s12 = g_input[dispatchThreadID.xy + tx     ].g;
    float s20 = g_input[dispatchThreadID.xy - tx + ty].g;
    float s21 = g_input[dispatchThreadID.xy      + ty].g;
    float s22 = g_input[dispatchThreadID.xy + tx + ty].g;

    float sx = s00 + 2 * s10 + s20 - s02 - 2 * s12 - s22;
    float sy = s00 + 2 * s01 + s02 - s20 - 2 * s21 - s22;

    float dist = sqrt(sx * sx + sy * sy);
    bool edge = dist > outlineThreshHold ? true : false;
 
    if (edge) g_output[dispatchThreadID.xy] = outlineColor;
}
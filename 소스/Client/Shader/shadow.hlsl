#include "common.hlsl"

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, g_worldMatrix);
    output.positionH = mul(mul(output.positionW, g_lights[0].lightViewMatrix), g_lights[0].lightProjMatrix);
    return output;
}
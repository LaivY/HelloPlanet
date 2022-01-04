#include "common.hlsl"

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, worldMatrix);
    output.positionH = mul(output.positionW, viewMatrix);
    output.positionH = mul(output.positionH, projMatrix);
    output.color = input.color;
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    return input.color;
}

// --------------------------------------

PS_INPUT VSTextureMain(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, worldMatrix);
    output.positionH = mul(output.positionW, viewMatrix);
    output.positionH = mul(output.positionH, projMatrix);
    output.uv0 = input.uv0;
    return output;
}

float4 PSTextureMain(PS_INPUT input) : SV_TARGET
{    
    return g_texture.Sample(g_sampler, input.uv0);
}

// --------------------------------------

VS_INPUT VSBillboardMain(VS_INPUT input)
{
    VS_INPUT output;
    output.position = mul(input.position, worldMatrix);
    output.uv0 = input.uv0;
    return output;
}

[maxvertexcount(4)]
void GSBillboardMain(point VS_INPUT input[1], uint primID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> triStream)
{
    // y축으로만 회전하는 빌보드
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = eye - input[0].position.xyz;
    look.y = 0.0f;
    look = normalize(look);
    float3 right = cross(up, look);
    
    float hw = 0.5f * input[0].uv0.x;
    float hh = 0.5f * input[0].uv0.y;
    
    float4 position[4] =
    {
        float4(input[0].position.xyz + (hw * right) - (hh * up), 1.0f), // LB
        float4(input[0].position.xyz + (hw * right) + (hh * up), 1.0f), // LT
        float4(input[0].position.xyz - (hw * right) - (hh * up), 1.0f), // RB
        float4(input[0].position.xyz - (hw * right) + (hh * up), 1.0f)  // RT
    };
    
    float2 uv[4] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    PS_INPUT output = (PS_INPUT)0;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        output.positionW = position[i];
        output.positionH = mul(output.positionW, viewMatrix);
        output.positionH = mul(output.positionH, projMatrix);
        output.uv0 = uv[i];
        triStream.Append(output);
    }
}

float4 PSBillboardMain(PS_INPUT input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv0);
}

// --------------------------------------

PS_INPUT VSTerrainMain(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, worldMatrix);
    output.positionH = mul(output.positionW, viewMatrix);
    output.positionH = mul(output.positionH, projMatrix);
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;
    return output;
}

float4 PSTerrainMain(PS_INPUT input) : SV_TARGET
{
    float4 baseTextureColor = g_texture.Sample(g_sampler, input.uv0);
    float4 detailTextureColor = g_detailTexture.Sample(g_sampler, input.uv1);
    return lerp(baseTextureColor, detailTextureColor, 0.4f);
}

// --------------------------------------

VS_INPUT VSTerrainTessMain(VS_INPUT input)
{
    VS_INPUT output;
    output.position = mul(input.position, worldMatrix);
    output.normal = mul(float4(input.normal, 0.0f), worldMatrix).xyz;
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;
    return output;
}

float CalculateTessFactor(float3 position)
{
    float fDistToCamera = distance(position, eye);
    float s = saturate(fDistToCamera / 75.0f);
    return lerp(64.0f, 3.0f, s);
}

PatchTess TerrainTessConstantHS(InputPatch<VS_INPUT, 25> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess output;
    output.EdgeTess[0] = CalculateTessFactor(0.5f * (patch[0].position.xyz + patch[4].position.xyz));
    output.EdgeTess[1] = CalculateTessFactor(0.5f * (patch[0].position.xyz + patch[20].position.xyz));
    output.EdgeTess[2] = CalculateTessFactor(0.5f * (patch[4].position.xyz + patch[24].position.xyz));
    output.EdgeTess[3] = CalculateTessFactor(0.5f * (patch[20].position.xyz + patch[24].position.xyz));
    
    float3 center = float3(0, 0, 0);
    for (int i = 0; i < 25; ++i)
        center += patch[i].position.xyz;
    center /= 25.0f;
    output.InsideTess[0] = output.InsideTess[1] = CalculateTessFactor(center);

    return output;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(25)]
[patchconstantfunc("TerrainTessConstantHS")]
[maxtessfactor(64.0f)]
VS_INPUT HSTerrainTessMain(InputPatch<VS_INPUT, 25> p, uint i : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    VS_INPUT output;
    output.position = p[i].position;
    output.normal = p[i].normal;
    output.uv0 = p[i].uv0;
    output.uv1 = p[i].uv1;
    return output;
}

void BernsteinCoeffcient5x5(float t, out float fBernstein[5])
{
    float tInv = 1.0f - t;
    fBernstein[0] = tInv * tInv * tInv * tInv;
    fBernstein[1] = 4.0f * t * tInv * tInv * tInv;
    fBernstein[2] = 6.0f * t * t * tInv * tInv;
    fBernstein[3] = 4.0f * t * t * t * tInv;
    fBernstein[4] = t * t * t * t;
}

float3 CubicBezierSum5x5(OutputPatch<VS_INPUT, 25> patch, float2 uv)
{
    // 4차 베지에 곡선 계수 계산
    float uB[5], vB[5];
    BernsteinCoeffcient5x5(uv.x, uB);
    BernsteinCoeffcient5x5(uv.y, vB);
    
    // 4차 베지에 곡면 계산
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 5; ++i)
    {
        float3 subSum = float3(0.0f, 0.0f, 0.0f);
        for (int j = 0; j < 5; ++j)
        {
            subSum += uB[j] * patch[i * 5 + j].position;
        }
        sum += vB[i] * subSum;
    }
    return sum;
}

[domain("quad")]
PS_INPUT DSTerrainTessMain(PatchTess patchTess, float2 uv : SV_DomainLocation, const OutputPatch<VS_INPUT, 25> patch)
{
    PS_INPUT output;
    
    // 위치 좌표(베지에 곡면)
    output.positionW = float4(CubicBezierSum5x5(patch, uv), 1.0f);
    output.positionH = mul(output.positionW, viewMatrix);
    output.positionH = mul(output.positionH, projMatrix);
    
    // 그림자 텍스쳐 읽을 좌표
    output.shadowPosH = mul(output.positionW, lightViewMatrix);
    output.shadowPosH = mul(output.shadowPosH, lightProjMatrix);
    output.shadowPosH = mul(output.shadowPosH, NDCToTextureMatrix);
    
    // 노말 보간
    output.normalW = lerp(lerp(patch[0].normal, patch[4].normal, uv.x), lerp(patch[20].normal, patch[24].normal, uv.x), uv.y);
    
    // 텍스쳐 좌표 보간
    output.uv0 = lerp(lerp(patch[0].uv0, patch[4].uv0, uv.x), lerp(patch[20].uv0, patch[24].uv0, uv.x), uv.y);
    output.uv1 = lerp(lerp(patch[0].uv1, patch[4].uv1, uv.x), lerp(patch[20].uv1, patch[24].uv1, uv.x), uv.y);
    
    return output;
}

float4 PSTerrainTessMain(PS_INPUT input) : SV_TARGET
{   
    // 텍스쳐 색깔
    float4 baseTextureColor = g_texture.Sample(g_sampler, input.uv0);
    float4 detailTextureColor = g_detailTexture.Sample(g_sampler, input.uv1);
    float4 texColor = lerp(baseTextureColor, detailTextureColor, 0.4f);
    
    // 그림자 계산
    float shadowFacter = CalcShadowFactor(input.shadowPosH); // 그림자이면 0, 아니면 1 [0~1]
    
    return texColor * saturate(shadowFacter + 0.7f); // 그림자를 좀 흐리게 그림
}

float4 PSTerrainTessWireMain(PS_INPUT pin) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

// --------------------------------------

PS_INPUT VSShadowMain(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, worldMatrix);
    output.positionH = mul(output.positionW, lightViewMatrix);
    output.positionH = mul(output.positionH, lightProjMatrix);
    return output;
}

// --------------------------------------

PS_INPUT VSModelMain(VS_INPUT input)
{
    PS_INPUT output;
    output.positionW = mul(input.position, worldMatrix);
    output.positionH = mul(output.positionW, viewMatrix);
    output.positionH = mul(output.positionH, projMatrix);
    output.shadowPosH = mul(output.positionW, lightViewMatrix);
    output.shadowPosH = mul(output.shadowPosH, lightProjMatrix);
    output.shadowPosH = mul(output.shadowPosH, NDCToTextureMatrix);
    output.normalW = mul(input.normal, (float3x3)worldMatrix);
    output.color = input.color;
    return output;
}

float4 PSModelMain(PS_INPUT input) : SV_TARGET
{
    float4 lightColor = Lighting(materials[0], input.positionW.xyz, input.normalW, false, input.shadowPosH);
    return input.color + lightColor;
}
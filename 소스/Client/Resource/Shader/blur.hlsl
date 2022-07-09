cbuffer cbBlurFilter : register(b1)
{
	int	g_radius;
}

Texture2D g_input               : register(t0);
RWTexture2D<float4> g_output    : register(u0);

static const int g_blurRadius = 5;
static const float g_weights[11] = { 0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f };

// 쓰레드 공유 데이터
// 256개 + 양쪽 끝에 반지름 * 2 만큼 더 필요함
groupshared float4 g_cache[256 + g_blurRadius * 2];

// 가로 블러
[numthreads(256, 1, 1)]
void HorzBlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    // SV_GroupThreadID     : 그룹 내에서 쓰레드의 좌표( (0, 0, 0) ~ (255, 0, 0) ) 
    // SV_DispatchThreadID  : 전체 쓰레드에서 쓰레드의 좌표( (0, 0, 0) ~ (WIDTH, HEIGHT, 0) )
    // g_cache에 블러링할 픽셀들을 넣고, g_cache를 블러링해서 g_output에 저장한다.

    // 텍스쳐 밖의 픽셀은 (0, 0, 0)으로 취급되기 때문에 따로 처리하지 않으면 테두리가 어두워진다.
    // 텍스쳐 밖 픽셀을 읽으려고 하는 쓰레드는 텍스쳐 가장자리에 있는 픽셀을 읽게 설정한다.
    if (groupThreadID.x < g_blurRadius)
    {
        int x = max(dispatchThreadID.x - g_blurRadius, 0);
        g_cache[groupThreadID.x] = g_input[int2(x, dispatchThreadID.y)];
    }
    if (groupThreadID.x >= 256 - g_blurRadius)
    {
        int x = min(dispatchThreadID.x + g_blurRadius, g_input.Length.x - 1);
        g_cache[groupThreadID.x + 2 * g_blurRadius] = g_input[int2(x, dispatchThreadID.y)];
    }
    g_cache[groupThreadID.x + g_blurRadius] = g_input[min(dispatchThreadID.xy, g_input.Length.xy - 1)];

    // 모든 쓰레드가 종료될 때까지 대기
    GroupMemoryBarrierWithGroupSync();

    uint width, height, numberOfLevels;
    g_input.GetDimensions(0, width, height, numberOfLevels);
    if (distance(dispatchThreadID.xy, int2(width / 2, height / 2)) < g_radius)
    {
        g_output[dispatchThreadID.xy] = g_input[dispatchThreadID.xy];
        return;
    }

    // 블러링
    float4 blurColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = -g_blurRadius; i <= g_blurRadius; ++i)
    {
        int k = groupThreadID.x + g_blurRadius + i;
        blurColor += g_weights[g_blurRadius + i] * g_cache[k];
    }
    g_output[dispatchThreadID.xy] = blurColor;
}

// 세로 블러
[numthreads(1, 256, 1)]
void VertBlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    if (groupThreadID.y < g_blurRadius)
    {
        int y = max(dispatchThreadID.y - g_blurRadius, 0);
        g_cache[groupThreadID.y] = g_input[int2(dispatchThreadID.x, y)];
    }
    if (groupThreadID.y >= 256 - g_blurRadius)
    {
        int y = min(dispatchThreadID.y + g_blurRadius, g_input.Length.y - 1);
        g_cache[groupThreadID.y + 2 * g_blurRadius] = g_input[int2(dispatchThreadID.x, y)];
    }
    g_cache[groupThreadID.y + g_blurRadius] = g_input[min(dispatchThreadID.xy, g_input.Length.xy - 1)];

    GroupMemoryBarrierWithGroupSync();
	
    uint width, height, numberOfLevels;
    g_input.GetDimensions(0, width, height, numberOfLevels);
    if (distance(dispatchThreadID.xy, int2(width / 2, height / 2)) < g_radius)
    {
        g_output[dispatchThreadID.xy] = g_input[dispatchThreadID.xy];
        return;
    }

    float4 blurColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = -g_blurRadius; i <= g_blurRadius; ++i)
    {
        int k = groupThreadID.y + g_blurRadius + i;
        blurColor += g_weights[i + g_blurRadius] * g_cache[k];
    }
    g_output[dispatchThreadID.xy] = blurColor;
}
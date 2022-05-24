cbuffer cbGameFramework : register(b0)
{
	int	g_fadeType;
	float g_fadeTime;
	float g_fadeTimer;
    float padding;
}

Texture2D g_input               : register(t0);
RWTexture2D<float4> g_output    : register(u0);

[numthreads(32, 32, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // 매개변수
    float t = 1.0f;

    // 페이드아웃(점점 어두워짐)
    if (g_fadeType == -1)
    {
        t = lerp(1.0f, 0.0f, g_fadeTimer / g_fadeTime);
    }

    // 페이드인(점점 밝아짐)
    else if (g_fadeType == 1)
    {
        t = lerp(0.0f, 1.0f, g_fadeTimer / g_fadeTime);
    }
    g_output[dispatchThreadID.xy] = g_input[dispatchThreadID.xy] * t;
}
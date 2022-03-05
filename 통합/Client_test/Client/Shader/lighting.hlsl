#define DIRECTIONAL_LIGHT   0
#define POINT_LIGHT         1
#define MAX_LIGHT           1

static const matrix toTextureMatrix =
{
	0.5f,  0.0f, 0.0f, 0.0f,
	0.0f, -0.5f, 0.0f, 0.0f,
	0.0f,  0.0f, 1.0f, 0.0f,
	0.5f,  0.5f, 0.0f, 1.0f
};

struct Light
{
	matrix lightViewMatrix;
	matrix lightProjMatrix;
	float3 position;
	float padding1;
	float3 direction;
	float padding2;
};

struct Material
{
	float4 baseColor;
};
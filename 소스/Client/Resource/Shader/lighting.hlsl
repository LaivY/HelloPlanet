#define DIRECTIONAL_LIGHT   0
#define POINT_LIGHT         1
#define MAX_LIGHT           3
#define SHADOWMAP_COUNT     4

struct Light
{
	matrix	lightViewMatrix;
	matrix 	lightProjMatrix;
	float3 	color;
	float 	padding1;
	float3 	direction;
	float 	padding2;
};

struct ShadowLight
{
    matrix	lightViewMatrix[SHADOWMAP_COUNT];
	matrix 	lightProjMatrix[SHADOWMAP_COUNT];
	float3 	color;
	float 	padding1;
	float3 	direction;
	float 	padding2;
};

struct Material
{
	float4 	baseColor;
	float3 	reflection;
	float 	roughness;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));
    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);
    return reflectPercent;
}

float3 BlinnPhong(Material material, float3 lightStrength, float3 lightVec, float3 normal, float3 toEye)
{
    const float m = (1.0f - material.roughness) * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(material.reflection, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor * roughnessFactor;

    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (material.baseColor.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light light, Material material, float3 normal, float3 toEye)
{   
    float3 lightVec = -light.direction;

    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.color * ndotl;

    return BlinnPhong(material, lightStrength, lightVec, normal, toEye);
}
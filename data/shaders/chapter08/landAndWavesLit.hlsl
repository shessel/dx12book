struct LightData
{
    float3 position;
    float falloffBegin;
    float3 direction;
    float falloffEnd;
    float3 color;
    float spotPower;
};

struct MaterialData
{
    float4 albedoColor;
    float3 fresnelR0;
    float roughness;
};

static const uint MAX_LIGHT_COUNT = 16;

struct PassData
{
    float4x4 view;
    float4x4 projection;
    float time;
    float dTime;
    uint directionalLightCount;
    uint pointLightCount;
    uint spotLightCount;
    float3 ambientLight;
    float3 cameraPositionW;
    LightData lightData[MAX_LIGHT_COUNT];
};

struct ObjectData
{
    float4x4 model;
};

ConstantBuffer<MaterialData> g_cbMaterial : register(b0);
ConstantBuffer<ObjectData> g_cbObject : register(b1);
ConstantBuffer<PassData> g_cbPass : register(b2);

struct VertexInput
{
    float4 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct VertexOutput
{
    float4 positionH : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
};

VertexOutput vs(VertexInput vIn)
{
    VertexOutput vOut;
    float4 position = vIn.position;
    position = mul(g_cbObject.model, position);
    vOut.positionW = position.xyz;

    position = mul(g_cbPass.view, position);
    position = mul(g_cbPass.projection, position);
    vOut.positionH = position;

    // works because in this demo model is just rotation -> orthogonal -> (M^-1)T == M
    vOut.normalW = mul((float3x3)g_cbObject.model, vIn.normal);

    return vOut;
}

float3 schlickFresnel(float cosTheta)
{
    float f = saturate(1.0f - cosTheta);
    return g_cbMaterial.fresnelR0 + (1.0f - g_cbMaterial.fresnelR0) * (f * f * f * f * f);
}

float3 computeBrdf(LightData light, float3 positionW, float3 normalW)
{
    float3 toCamera = g_cbPass.cameraPositionW - positionW;
    float3 cameraDirection = normalize(toCamera);

    float3 toLight = -light.direction;
    float3 lightDirection = normalize(toLight);

    float3 halfway = normalize(cameraDirection + lightDirection);

    float nDotL = max(dot(normalW, lightDirection), 0.0f);
    float3 ambientLight = g_cbMaterial.albedoColor.xyz * g_cbPass.ambientLight;
    float3 diffuseLight = g_cbMaterial.albedoColor.xyz;

    float m = (1.0f - g_cbMaterial.roughness) * 256.0f;
    float3 specularLight = schlickFresnel(dot(lightDirection, halfway)) * ((m + 8.0f) / 8.0f) * pow(dot(normalW, halfway), m);
    return ambientLight + nDotL * (diffuseLight + specularLight);
}

float3 computeDirectionalLights(float3 positionW, float3 normalW)
{
    float3 sumDirectionalLight = (float3) 0.0f;
    for (uint i = 0; i < g_cbPass.directionalLightCount; ++i)
    {
        sumDirectionalLight += computeBrdf(g_cbPass.lightData[i], positionW, normalW);
    }
    return sumDirectionalLight;
}

float4 ps(VertexOutput pIn) : SV_TARGET
{
    float3 normal = normalize(pIn.normalW);
    return float4(computeDirectionalLights(pIn.positionW, pIn.normalW), 1.0f);
}
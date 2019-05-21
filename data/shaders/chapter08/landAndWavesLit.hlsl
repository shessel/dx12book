struct LightData
{
    float3 positionW;
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

#include "../common/lighting.hlsl"

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

float4 ps(VertexOutput pIn) : SV_TARGET
{
    float3 normal = normalize(pIn.normalW);
    float3 toCamera = normalize(g_cbPass.cameraPositionW - pIn.positionW);
    return float4(computeLights(pIn.positionW, normal, toCamera), 1.0f);
}

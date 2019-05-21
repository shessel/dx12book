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
    float2 texCoordTransform0;
    float2 texCoordTransform1;
    float2 texCoordOffset;
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
    float2 texCoordTransform0;
    float2 texCoordTransform1;
    float2 texCoordOffset;
};

ConstantBuffer<MaterialData> g_cbMaterial : register(b0);
ConstantBuffer<ObjectData> g_cbObject : register(b1);
ConstantBuffer<PassData> g_cbPass : register(b2);

Texture2D g_tex : register(t0);

SamplerState g_samplerPointWrap : register(s0);
SamplerState g_samplerPointClamp : register(s1);
SamplerState g_samplerLinearWrap : register(s2);
SamplerState g_samplerLinearClamp : register(s3);
SamplerState g_samplerAnisotropicWrap : register(s4);
SamplerState g_samplerAnisotropicClamp : register(s5);

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
    float2 uv : TEXCOORD;
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

    float2x2 mat = float2x2(g_cbObject.texCoordTransform0, g_cbObject.texCoordTransform1);
    vOut.uv = mul(mat, vIn.uv) + g_cbObject.texCoordOffset;
    mat = float2x2(g_cbMaterial.texCoordTransform0, g_cbMaterial.texCoordTransform1);
    vOut.uv = mul(mat, vOut.uv) + g_cbMaterial.texCoordOffset;

    return vOut;
}

float4 ps(VertexOutput pIn) : SV_TARGET
{
    float3 normal = normalize(pIn.normalW);
    float3 toCamera = normalize(g_cbPass.cameraPositionW - pIn.positionW);
    return float4(computeLights(pIn.positionW, normal, toCamera), 1.0f) * g_tex.Sample(g_samplerLinearWrap, pIn.uv);
}

struct PassData
{
	float4x4 view;
	float4x4 projection;
	float time;
	float dTime;
};

struct ObjectData
{
	float4x4 model;
};

ConstantBuffer<ObjectData> g_cbObject : register(b0);
ConstantBuffer<PassData> g_cbPass : register(b1);

struct VertexInput
{
	float4 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct VertexOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VertexOutput vs(VertexInput vIn)
{
	VertexOutput vOut;
	//vIn.position.xy += 0.5f*sin(vIn.position.x)*sin(3.0f*g_cbPass.time);
	//vIn.position.z *= 0.6f + 0.4f*sin(2.0f*g_cbPass.time);
	vOut.position = mul(g_cbObject.model, vIn.position);
	vOut.position = mul(g_cbPass.view, vOut.position);
	vOut.position = mul(g_cbPass.projection, vOut.position);
	vOut.color = float4(vIn.uv, 0.0f, 1.0f);
	return vOut;
}

float4 ps(VertexOutput pIn) : SV_TARGET
{
	return pIn.color;
}
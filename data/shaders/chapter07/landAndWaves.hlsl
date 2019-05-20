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
    float3 color : COLOR;
};

struct VertexOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VertexOutput vs(VertexInput vIn)
{
	VertexOutput vOut;
	vOut.position = mul(g_cbObject.model, vIn.position);
	vOut.position = mul(g_cbPass.view, vOut.position);
	vOut.position = mul(g_cbPass.projection, vOut.position);
	vOut.color = float4(vIn.color, 1.0f);
	return vOut;
}

float4 ps(VertexOutput pIn) : SV_TARGET
{
	return pIn.color;
}
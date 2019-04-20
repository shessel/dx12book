struct CBData
{
	float4x4 model;
	float4x4 view;
	float4x4 projection;
	float time;
};

ConstantBuffer<CBData> g_cbData : register(b0);

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
	vIn.position.xy += 0.5f*sin(vIn.position.x)*sin(3.0f*g_cbData.time);
	vIn.position.z *= 0.6f + 0.4f*sin(2.0f*g_cbData.time);
	vOut.position = mul(g_cbData.view, vIn.position);
	vOut.position = mul(g_cbData.projection, vOut.position);
	vOut.color = float4(vIn.color, 1.0f);
	return vOut;
}

float4 ps(VertexOutput pIn) : SV_TARGET
{
	return pIn.color;
}
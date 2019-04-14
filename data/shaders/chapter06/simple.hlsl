struct CBData
{
	float4x4 mat;
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
	vOut.position = mul(g_cbData.mat, vIn.position);
	vOut.color = float4(vIn.color, 1.0f);
	return vOut;
}

float4 ps(VertexOutput pIn) : SV_TARGET
{
	return pIn.color;
}
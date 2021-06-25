struct VS_INPUT
{
	float3 pos : POSITION;
	float4 color : COLOR;
	float2 texcoord : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 pos: SV_POSITION;
	float4 color : COLOR;
	float2 texcoord : TEXCOORD;
};cbuffer ConstantBuffer : register(b0)
{
	float4 colorMultiplier;
	float4x4 wvpMat;
};
VS_OUTPUT mainCB(VS_INPUT input)
{
	VS_OUTPUT output;

	output.pos = mul(wvpMat, float4(input.pos, 1.0f));
	output.color = input.color * colorMultiplier;
	output.texcoord = input.texcoord;

	return output;
}
VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.pos = float4(input.pos, 1.0f);
	output.color = input.color;
	output.texcoord = input.texcoord;

	return output;
}
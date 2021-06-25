struct VS_OUTPUT
{
	float4 pos: SV_POSITION;
	float4 color : COLOR;
	float2 texcoord : TEXCOORD;
};

#ifdef TEXTURE
Texture2D t1: register(t0);
SamplerState s1 : register(s0);
#endif

float4 main(VS_OUTPUT input) : SV_TARGET
{
#ifdef TEXTURE
	return t1.Sample(s1, input.texcoord);
#else
	return input.color;
#endif
}

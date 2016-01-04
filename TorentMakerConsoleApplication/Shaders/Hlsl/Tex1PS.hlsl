
sampler TexSampler : register(s0);

Texture2D Tex : register(t0);

struct PsInput {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(PsInput input) : SV_TARGET{
	float4 colorTmp = Tex.Sample(TexSampler, input.tex);
	return colorTmp;
}
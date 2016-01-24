
cbuffer C0 : register(b0) {
	float4 color;
}

struct PsInput {
	float4 pos : SV_POSITION;
	float alpha : TEXCOORD0;
};

float4 main(PsInput input) : SV_TARGET{
	float4 colorTmp = color;

	colorTmp.a *= input.alpha;

	return colorTmp;
}
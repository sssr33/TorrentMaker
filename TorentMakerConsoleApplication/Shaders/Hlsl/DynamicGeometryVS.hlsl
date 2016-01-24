
cbuffer C0 : register(b0) {
	matrix Transform;
}

struct VsInput {
	float4 pos : POSITION;
	float alpha : TEXCOORD0;
};

struct PsInput {
	float4 pos : SV_POSITION;
	float alpha : TEXCOORD0;
};

PsInput main(VsInput input) {
	PsInput output;

	output.pos = mul(input.pos, Transform);
	output.alpha = input.alpha;

	return output;
}
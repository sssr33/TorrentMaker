
static const float2 TexScale = float2(1.0f, -1.0f);
static const float2 TexTranslate = float2(0.5f, 0.5f);

// Triangle strip
static const float2 QuadStrip[4] = { 
	float2(-0.5f, -0.5f), 
	float2(-0.5f, 0.5f), 
	float2(0.5f, -0.5f), 
	float2(0.5f, 0.5f) 
};

cbuffer C0 : register(b0) {
	matrix Transform;
}

struct VsInput {
	float vertexId : POSITION;
};

struct PsInput {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

PsInput main(VsInput input) {
	PsInput output;

	output.pos = float4(QuadStrip[input.vertexId], 0.0f, 1.0f);
	output.pos = mul(output.pos, Transform);

	output.tex = QuadStrip[input.vertexId] * TexScale + TexTranslate;

	return output;
}

// 4 rows 3 columns
// last row is for uv -= 0.5
static const float4x3 yuvToRgb = {
	1.0f, 1.0f, 1.0f,
	0.0f, -0.395f, 2.032f,
	1.140f, -0.581f, 0.0f,
	-0.57f, 0.488f, -1.016f
};

// 4 rows 3 columns
// last row is for uv -= 0.5
static const float4x3 yuvToRgbHd = {
	1.0f, 1.0f, 1.0f,
	0.0f, -0.2153f, 2.1324f,
	1.2803f, -0.3806f, 0.0f,
	-0.64015f, 0.29795f, -1.0662f
};

sampler TexSampler : register(s0);

Texture2D TexY : register(t0);
Texture2D TexU : register(t1);
Texture2D TexV : register(t2);

struct PsInput {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(PsInput input) : SV_TARGET
{
	float3 rgb;
	float4 yuv;

	yuv.x = TexY.Sample(TexSampler, input.tex).r;
	yuv.y = TexU.Sample(TexSampler, input.tex).r;
	yuv.z = TexV.Sample(TexSampler, input.tex).r;
	yuv.w = 1.0f;

	rgb = mul(yuv, yuvToRgb);

	return float4(rgb, 1.0f);
}
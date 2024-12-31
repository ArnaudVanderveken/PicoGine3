/* @metadata
{
	"shader_model": "6_0",
	"entry_points": ["VSMain", "PSMain"]
}
*/

// STRUCTS
struct VSInput
{
#if defined(_VK)
	[[vk::location(0)]]
#endif
	float2 positionCS : POSITION;
#if defined(_VK)
	[[vk::location(1)]]
#endif
	float3 color : COLOR;
#if defined(_VK)
	[[vk::location(2)]]
#endif
	float2 texcoord : TEXCOORD;
};

struct VSOutput
{
	float4 positionCS : SV_POSITION;
#if defined(_VK)
	[[vk::location(0)]]
#endif
	float3 color : COLOR;
#if defined(_VK)
	[[vk::location(1)]]
#endif
	float2 texcoord : TEXCOORD;
};

struct UBO
{
	float4x4 modelMat;
	float4x4 viewMat;
	float4x4 projectionMat;
};

cbuffer ubo : register(b0, space0) { UBO ubo; }

Texture2D texture : register(t1);
SamplerState samplerState : register(s1);

// STAGES
VSOutput VSMain(VSInput input)
{
	VSOutput output = (VSOutput)0;

	output.positionCS = mul(ubo.projectionMat, mul(ubo.viewMat, mul(ubo.modelMat, float4(input.positionCS, 0.0f, 1.0f))));
	output.color = input.color;
	output.texcoord = input.texcoord;

	return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
	return texture.Sample(samplerState, input.texcoord);
}
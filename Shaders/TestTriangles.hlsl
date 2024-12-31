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
};

struct VSOutput
{
	float4 positionCS : SV_POSITION;
#if defined(_VK)
	[[vk::location(0)]]
#endif
	float3 color : COLOR;
};

struct UBO
{
	float4x4 modelMat;
	float4x4 viewMat;
	float4x4 projectionMat;
};

cbuffer ubo : register(b0, space0) { UBO ubo; }

// STAGES
VSOutput VSMain(VSInput input)
{
	VSOutput output = (VSOutput)0;

	output.positionCS = mul(ubo.projectionMat, mul(ubo.viewMat, mul(ubo.modelMat, float4(input.positionCS, 0.0f, 1.0f))));
	output.color = input.color;

	return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
	return float4(input.color, 1.0f);
}
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

// STAGES
VSOutput VSMain(VSInput input)
{
	VSOutput output = (VSOutput)0;

	output.positionCS = float4(input.positionCS, 0.0f, 1.0f);
	output.color = input.color;

	return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
	return float4(input.color, 1.0f);
}
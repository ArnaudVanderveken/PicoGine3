/* @metadata
{
	"shader_model": "6_0",
	"entry_points": ["VSMain", "PSMain"]
}
*/

// GLOBALS
static const float2 g_Positions[3] = {
	float2(0.0f, -0.5f),
	float2(0.5f, 0.5f),
	float2(-0.5f, 0.5f)
};

static const float3 g_Colors[3] = {
	float3(1.0f, 0.0f, 0.0f),
	float3(0.0f, 1.0f, 0.0f),
	float3(0.0f, 0.0f, 1.0f)
};

// STRUCTS
struct VSInput
{
	[[vk::location(0)]] float2 positionCS : POSITION;
	[[vk::location(1)]] float3 color 	  : COLOR;
};

struct VSOutput
{
						float4 positionCS : SV_POSITION;
	[[vk::location(0)]] float3 color : COLOR;
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
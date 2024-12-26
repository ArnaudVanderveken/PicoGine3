/* @metadata
{
	"shader_model": "6_0",
	"entry_points": ["VSMain", "PSMain"]
}
*/

// GLOBALS
const float2 g_Positions[3] = {
	float2(0.0f, -0.5f),
	float2(0.5f, 0.5f),
	float2(-0.5f, 0.5f)
};

const float3 g_Colors[3] = {
	float3(1.0f, 0.0f, 0.0f),
	float3(0.0f, 1.0f, 0.0f),
	float3(0.0f, 0.0f, 1.0f)
};

// STRUCTS
struct VSOutput
{
	float4 positionCS : SV_POSITION;
	float3 color : COLOR0;
};

// STAGES
VSOutput VSMain(uint vertexIndex : SV_VertexID)
{
	VSOutput output = (VSOutput)0;

	output.positionCS = float4(g_Positions[vertexIndex], 0.0f, 1.0f);
	output.color = g_Colors[vertexIndex];

	return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
	return float4(input.color, 1.0f);
}
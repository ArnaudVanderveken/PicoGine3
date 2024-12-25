/* @metadata
{
	"shader_model": "6_0",
	"entry_points": ["VSMain", "PSMain"]
}
*/

float4 VSMain(float3 position : POSITION) : SV_POSITION
{
	return float4(position, 1.0f);
}

float4 PSMain(float4 positionCS : SV_POSITION) : SV_TARGET
{
	return float4(1.0f, 1.0f, 0.0f, 1.0f);
}
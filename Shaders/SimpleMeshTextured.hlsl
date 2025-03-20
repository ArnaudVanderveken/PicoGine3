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
	float3 positionOS : POSITION;
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

struct PerFrameUBO
{
	row_major float4x4 viewMat;
	row_major float4x4 projMat;
	row_major float4x4 viewInvMat;
	row_major float4x4 projInvMat;
	row_major float4x4 viewProjMat;
	row_major float4x4 viewProjInvMat;
};
cbuffer ubo : register(b0, space0) { PerFrameUBO ubo; }

struct PushConstants
{
    row_major float4x4 modelMat;
};
#if defined(_VK)
[[vk::push_constant]]
#else //DX12
[[rootconstant(0)]]
#endif
PushConstants push;

Texture2D texture : register(t1);
SamplerState samplerState : register(s1);

// STAGES
VSOutput VSMain(VSInput input)
{
	VSOutput output = (VSOutput)0;

	output.positionCS = mul(mul(float4(input.positionOS, 1.0f), push.modelMat), ubo.viewProjMat);
	output.color = input.color;
	output.texcoord = input.texcoord;

	return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
	return texture.Sample(samplerState, input.texcoord);
}
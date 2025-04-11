#define SHADOW_DEPTH_BIAS 0.00005f
struct Light {
    float4 position;
    float4 color;
};
cbuffer ConstantBuffer: register(b0)
{
    float4x4 mwpMatrix;
    float4x4 shadowMatrix;
    Light light;
};
Texture2D g_texture : register(t0);
Texture2D g_shadow_map : register(t1);
SamplerState g_sampler : register(s0);
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv: TEXCOORD;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
};
PSInput VSMain(float4 position : POSITION, float4 normal: NORMAL, float4 ambient : COLOR0, float4 diffuse : COLOR1,  float4 emissive : COLOR2, float4 texcoords : TEXCOORD)
{
    PSInput result;
    result.position = mul(mwpMatrix, position);
    result.color = ambient;
    result.uv = texcoords.xy;
    result.world_position = position.xyz;
    result.normal = normal.xyz;
    return result;
}
PSInput VSShadowMap(float4 position : POSITION, float4 normal: NORMAL, float4 ambient : COLOR0, float4 diffuse : COLOR1,  float4 emissive : COLOR2, float4 texcoords : TEXCOORD)
{
    PSInput result;
    result.position = mul(shadowMatrix, position);
    result.color = ambient;
    result.uv = texcoords.xy;
    result.world_position = position.xyz;
    result.normal = normal.xyz;
    return result;
}
float4 GetLambertianShading(PSInput input, Light light) {
    float3 to_light = light.position.xyz - input.world_position;
    return saturate(dot(input.normal, normalize(to_light))) * light.color;
}
float CalcUnshadowedAmount(float3 world_position) {
    float4 light_space_position = float4(world_position, 1.0f);
    light_space_position = mul(shadowMatrix, light_space_position);
    light_space_position.xyz /= light_space_position.w;

    float2 vShadowTexCoord = 0.5f * light_space_position.xy + 0.5;
    vShadowTexCoord.y = 1.0f - vShadowTexCoord.y;

    float vLightSpaceDepth = light_space_position.z - SHADOW_DEPTH_BIAS;
    return (g_shadow_map.Sample(g_sampler, vShadowTexCoord) >= vLightSpaceDepth) ? 1.0f : 0.5f;
}
float4 PSMain(PSInput input) : SV_TARGET
{
    float4 diffuse = input.color;
    return diffuse * (1 + GetLambertianShading(input, light)) * CalcUnshadowedAmount(input.world_position) / 2.0f;
}
float4 PSMain_texture(PSInput input) : SV_TARGET
{
    float4 diffuse = g_texture.Sample(g_sampler, input.uv);
    return diffuse * (1 + GetLambertianShading(input, light)) * CalcUnshadowedAmount(input.world_position) / 2.0f;
}
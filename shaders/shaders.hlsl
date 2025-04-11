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
float4 GetLambertianShading(PSInput input, Light light) {
    float3 to_light = light.position.xyz - input.world_position;
    return saturate(dot(input.normal, normalize(to_light))) * light.color;
}
float4 PSMain(PSInput input) : SV_TARGET
{
    float4 diffuse = input.color;
    return diffuse * (1 + GetLambertianShading(input, light)) / 2.0f;
}
float4 PSMain_texture(PSInput input) : SV_TARGET
{
    float4 diffuse = g_texture.Sample(g_sampler, input.uv);
    return diffuse * (1 + GetLambertianShading(input, light)) / 2.0f;
}
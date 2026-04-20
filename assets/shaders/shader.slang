#include "shader_constants.h"

struct VSInput
{
  float3 Pos;
  float3 Normal;
  float2 UV;
};

Sampler2D textures[];

struct ShaderData
{
  float4x4 projection;
  float4x4 view;
  float4x4 model[INSTANCE_COUNT];
  float4   lightPos;
};

struct VSOutput
{
  float4   Pos : SV_POSITION;
  float3   Normal;
  float2   UV;
  float3   LightVec;
  float3   ViewVec;
  uint32_t InstanceIndex;
};

[shader("vertex")]
VSOutput main(VSInput input, uniform ShaderData* shaderData, uint instanceIndex: SV_VulkanInstanceID)
{
  VSOutput output;
  float4x4 modelMat = shaderData->model[instanceIndex];

  output.Normal        = mul((float3x3)mul(shaderData->view, modelMat), input.Normal);
  output.UV            = input.UV;
  output.Pos           = mul(shaderData->projection, mul(shaderData->view, mul(modelMat, float4(input.Pos.xyz, 1.0))));
  output.InstanceIndex = instanceIndex;

  // Calculate view vectors required for lighting
  float4 fragPos  = mul(mul(shaderData->view, modelMat), float4(input.Pos.xyz, 1.0));
  output.LightVec = shaderData->lightPos.xyz - fragPos.xyz;
  output.ViewVec  = -fragPos.xyz;
  return output;
}

[shader("fragment")]
float4 main(VSOutput input)
{
  float3 N = normalize(input.Normal);
  float3 L = normalize(input.LightVec);
  float3 V = normalize(input.ViewVec);
  float3 H = normalize(L + V);

  float NdotL = max(dot(N, L), 0.0);
  float NdotV = max(dot(N, V), 0.0);
  float NdotH = max(dot(N, H), 0.0);
  float LdotH = max(dot(L, H), 0.0);

  // Constants
  const float PI        = 3.14159265359;
  const float roughness = 0.05;
  const float F0        = 0.04;

  // Schlick Fresnel
  float oneMinusLdotH  = 1.0 - LdotH;
  float oneMinusLdotH5 = oneMinusLdotH * oneMinusLdotH * oneMinusLdotH * oneMinusLdotH * oneMinusLdotH;
  float F              = F0 + (1.0 - F0) * oneMinusLdotH5;

  // Burley diffuse
  float  FD90 = 0.5 + 2.0 * roughness * LdotH * LdotH;
  float  tL = 1.0 - NdotL, tV = 1.0 - NdotV;
  float  tL2 = tL * tL, tV2 = tV * tV;
  float  FL          = 1.0 + (FD90 - 1.0) * tL2 * tL2 * tL;
  float  FV          = 1.0 + (FD90 - 1.0) * tV2 * tV2 * tV;
  float  diffuseBRDF = (1.0 / PI) * FL * FV * (1.0 - F);
  float3 diffuse     = diffuseBRDF * NdotL;

  // GGX specular
  float r1       = roughness + 1.0;
  float k        = (r1 * r1) / 8.0;
  float G1_NdotL = NdotL / (NdotL * (1.0 - k) + k);
  float G1_NdotV = NdotV / (NdotV * (1.0 - k) + k);
  float G        = G1_NdotL * G1_NdotV;

  float alpha2   = roughness * roughness;
  float NdotH2   = NdotH * NdotH;
  float denomGGX = NdotH2 * (alpha2 - 1.0) + 1.0;
  float D        = alpha2 / (PI * denomGGX * denomGGX);

  float specular = (F * G * D) / (4.0 * NdotL * NdotV + 1e-5);

  // Material sampling and final color
  float3 texColor   = textures[NonUniformResourceIndex(input.InstanceIndex)].Sample(input.UV).rgb;
  float3 finalColor = diffuse * texColor + specular;

  return float4(finalColor, 1.0);
}

//==============================================================================
// pb3u4nt0_ld.hlsl - Skinned mesh shader, 4 bone influences, with lighting
//==============================================================================
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_SKIN4 input)
{
    VS_OUTPUT output;
    
    // Get bone indices from BlendIndices (D3DCOLOR format, BGRA order)
    int bone0Base = GetBoneRegisterBase(input.BlendIndices.z);
    int bone1Base = GetBoneRegisterBase(input.BlendIndices.y);
    int bone2Base = GetBoneRegisterBase(input.BlendIndices.x);
    int bone3Base = GetBoneRegisterBase(input.BlendIndices.w);
    
    // Weights: x = weight0, y = weight1, z = weight2, weight3 = 1 - sum
    float weight0 = input.BlendWeight.x;
    float weight1 = input.BlendWeight.y;
    float weight2 = input.BlendWeight.z;
    float weight3 = 1.0 - weight0 - weight1 - weight2;
    
    // Blend position
    float4 pos0 = TransformByBone(input.Position, bone0Base);
    float4 pos1 = TransformByBone(input.Position, bone1Base);
    float4 pos2 = TransformByBone(input.Position, bone2Base);
    float4 pos3 = TransformByBone(input.Position, bone3Base);
    float4 skinnedPos = pos0 * weight0 + pos1 * weight1 + pos2 * weight2 + pos3 * weight3;
    
    // Transform to clip space
    output.Position = mul(skinnedPos, ViewProj);
    
    // Blend normal
    float3 nrm0 = TransformNormalByBone(input.Normal, bone0Base);
    float3 nrm1 = TransformNormalByBone(input.Normal, bone1Base);
    float3 nrm2 = TransformNormalByBone(input.Normal, bone2Base);
    float3 nrm3 = TransformNormalByBone(input.Normal, bone3Base);
    float3 skinnedNormal = normalize(nrm0 * weight0 + nrm1 * weight1 + nrm2 * weight2 + nrm3 * weight3);
    
    // Calculate lighting
    output.Color    = CalcLighting(skinnedNormal);
    output.Specular = CalcSpecular(skinnedNormal);
    
    // Transform texture coordinates
    float4 uv = float4(input.TexCoord, 0.0, 1.0);
    
#ifdef USE_UVMAT0
    output.TexCoord = mul(uv, UVMat0).xy;
#elif defined(USE_UVMAT1)
    output.TexCoord = mul(uv, UVMat1).xy;
#elif defined(USE_UVMAT2)
    output.TexCoord = mul(uv, UVMat2).xy;
#else
    output.TexCoord = input.TexCoord;
#endif
    
    return output;
}

//==============================================================================
// pu4nt0_ld.hlsl - Skinned mesh shader, 1 bone influence, with lighting
//==============================================================================
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_SKIN1 input)
{
    VS_OUTPUT output;
    
    // Get bone index from BlendIndices (D3DCOLOR format)
    // BlendIndices.z is the first index (BGRA -> z=R channel in ZYXW swizzle)
    int boneBase = GetBoneRegisterBase(input.BlendIndices.z);
    
    // Transform position by bone
    float4 skinnedPos = TransformByBone(input.Position, boneBase);
    
    // Transform to clip space
    output.Position = mul(skinnedPos, ViewProj);
    
    // Transform normal by bone for lighting
    float3 skinnedNormal = TransformNormalByBone(input.Normal, boneBase);
    skinnedNormal = normalize(skinnedNormal);
    
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

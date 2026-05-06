//==============================================================================
// pb2u4nt0_ld_outline.hlsl - Outline pass, 3 bone influences
// Inverted hull technique: extrude vertices along normals, cull=CW, black
//==============================================================================
#include "common.hlsli"

static const float OUTLINE_WIDTH = 0.025;

VS_OUTPUT main(VS_INPUT_SKIN3 input)
{
    VS_OUTPUT output;

    int bone0Base = GetBoneRegisterBase(input.BlendIndices.z);
    int bone1Base = GetBoneRegisterBase(input.BlendIndices.y);
    int bone2Base = GetBoneRegisterBase(input.BlendIndices.x);

    float weight0 = input.BlendWeight.x;
    float weight1 = input.BlendWeight.y;
    float weight2 = 1.0 - weight0 - weight1;

    // Blend position
    float4 pos0 = TransformByBone(input.Position, bone0Base);
    float4 pos1 = TransformByBone(input.Position, bone1Base);
    float4 pos2 = TransformByBone(input.Position, bone2Base);
    float4 skinnedPos = pos0 * weight0 + pos1 * weight1 + pos2 * weight2;

    // Blend normal
    float3 nrm0 = TransformNormalByBone(input.Normal, bone0Base);
    float3 nrm1 = TransformNormalByBone(input.Normal, bone1Base);
    float3 nrm2 = TransformNormalByBone(input.Normal, bone2Base);
    float3 skinnedNormal = normalize(nrm0 * weight0 + nrm1 * weight1 + nrm2 * weight2);

    // Extrude vertex outward along normal to create outline shell
    skinnedPos.xyz += skinnedNormal * OUTLINE_WIDTH;

    output.Position  = mul(skinnedPos, ViewProj);
    output.Color     = float4(0.0, 0.0, 0.0, 1.0);
    output.TexCoord  = input.TexCoord;

    return output;
}

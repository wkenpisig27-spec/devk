//==============================================================================
// pb1u4nt0_ld_outline.hlsl - Outline pass, 2 bone influences
// Inverted hull: extrude in clip space (constant pixel width), tinted color.
//==============================================================================
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_SKIN2 input)
{
    VS_OUTPUT output;

    int bone0Base = GetBoneRegisterBase(input.BlendIndices.z);
    int bone1Base = GetBoneRegisterBase(input.BlendIndices.y);

    float weight0 = input.BlendWeight.x;
    float weight1 = 1.0 - weight0;

    float4 pos0 = TransformByBone(input.Position, bone0Base);
    float4 pos1 = TransformByBone(input.Position, bone1Base);
    float4 skinnedPos = pos0 * weight0 + pos1 * weight1;

    float3 nrm0 = TransformNormalByBone(input.Normal, bone0Base);
    float3 nrm1 = TransformNormalByBone(input.Normal, bone1Base);
    float3 skinnedNormal = normalize(nrm0 * weight0 + nrm1 * weight1);

    output.Position = OutlineClipPos(skinnedPos.xyz, skinnedNormal);
    output.Color    = OUTLINE_COLOR;
    output.TexCoord = input.TexCoord;

    return output;
}

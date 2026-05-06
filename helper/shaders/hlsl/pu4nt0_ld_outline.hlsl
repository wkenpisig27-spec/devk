//==============================================================================
// pu4nt0_ld_outline.hlsl - Outline pass, 1 bone influence
// Inverted hull: extrude in clip space (constant pixel width), tinted color.
//==============================================================================
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_SKIN1 input)
{
    VS_OUTPUT output;

    int boneBase = GetBoneRegisterBase(input.BlendIndices.z);

    float4 skinnedPos    = TransformByBone(input.Position, boneBase);
    float3 skinnedNormal = normalize(TransformNormalByBone(input.Normal, boneBase));

    output.Position = OutlineClipPos(skinnedPos.xyz, skinnedNormal);
    output.Color    = OUTLINE_COLOR;
    output.TexCoord = input.TexCoord;

    return output;
}

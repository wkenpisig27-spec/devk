//==============================================================================
// pu4nt0_ld_outline.hlsl - Outline pass, 1 bone influence
// Inverted hull technique: extrude vertices along normals, cull=CW, black
//==============================================================================
#include "common.hlsli"

static const float OUTLINE_WIDTH = 0.025;

VS_OUTPUT main(VS_INPUT_SKIN1 input)
{
    VS_OUTPUT output;

    int boneBase = GetBoneRegisterBase(input.BlendIndices.z);

    float4 skinnedPos    = TransformByBone(input.Position, boneBase);
    float3 skinnedNormal = normalize(TransformNormalByBone(input.Normal, boneBase));

    // Extrude vertex outward along normal to create outline shell
    skinnedPos.xyz += skinnedNormal * OUTLINE_WIDTH;

    output.Position  = mul(skinnedPos, ViewProj);
    output.Color     = float4(0.0, 0.0, 0.0, 1.0);
    output.TexCoord  = input.TexCoord;

    return output;
}

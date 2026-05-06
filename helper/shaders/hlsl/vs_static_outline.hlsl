//==============================================================================
// vs_static_outline.hlsl - Outline pass for static mesh items
// Works for all static VS types: PNT*, PNDT* (outline only reads POSITION+NORMAL)
// Inverted hull: extrude in clip space (constant pixel width), tinted color.
//==============================================================================
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_PNT input)
{
    VS_OUTPUT output;

    float3 normal = normalize(input.Normal);

    output.Position = OutlineClipPos(input.Position, normal);
    output.Color    = OUTLINE_COLOR;
    output.TexCoord = input.TexCoord;

    return output;
}

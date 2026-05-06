//==============================================================================
// vs_static_outline.hlsl - Inverted-hull outline pass for static mesh items
// Works for all static VS types: PNT*, PNDT* (outline only reads POSITION+NORMAL)
// ViewProj (c1-c4) already contains transposed (ViewProj * World) set by engine.
//==============================================================================
#include "common.hlsli"

static const float OUTLINE_WIDTH = 0.025;

VS_OUTPUT main(VS_INPUT_PNT input)
{
    VS_OUTPUT output;

    // Extrude position outward along the vertex normal
    float3 normal = normalize(input.Normal);
    float4 pos = float4(input.Position + normal * OUTLINE_WIDTH, 1.0);

    // Transform to clip space (ViewProj already folds in world transform)
    output.Position = mul(pos, ViewProj);

    // Solid black outline
    output.Color    = float4(0.0, 0.0, 0.0, 1.0);
    output.TexCoord = input.TexCoord;

    return output;
}

//==============================================================================
// vs_pndt0.hlsl - Static mesh shader with vertex color (no lighting)
// Position, Normal, Diffuse color, TexCoord
//==============================================================================
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_PNDT input)
{
    VS_OUTPUT output;
    
    // Transform position to clip space
    float4 worldPos = float4(input.Position, 1.0);
    output.Position = mul(worldPos, ViewProj);
    
    // Use vertex color directly
    output.Color    = input.Color;
    
    // Pass through texture coordinates
    output.TexCoord = input.TexCoord;
    
    return output;
}

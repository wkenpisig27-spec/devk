//==============================================================================
// vs_pnt0_t0uvmat.hlsl - Static mesh shader with UV matrix transform (no lighting)
//==============================================================================
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_PNT input)
{
    VS_OUTPUT output;
    
    // Transform position to clip space
    float4 worldPos = float4(input.Position, 1.0);
    output.Position = mul(worldPos, ViewProj);
    
    // No lighting - use white; no specular for unlit geometry
    output.Color    = float4(1.0, 1.0, 1.0, 1.0);
    output.Specular = float4(0.0, 0.0, 0.0, 0.0);
    
    // Transform texture coordinates by UV matrix
    float4 uv = float4(input.TexCoord, 0.0, 1.0);
    output.TexCoord = mul(uv, UVMat0).xy;
    
    return output;
}

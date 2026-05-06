//==============================================================================
// vs_pnt0_ld_t0uvmat.hlsl - Static mesh shader with lighting and UV matrix
//==============================================================================
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_PNT input)
{
    VS_OUTPUT output;
    
    // Transform position to clip space
    float4 worldPos = float4(input.Position, 1.0);
    output.Position = mul(worldPos, ViewProj);
    
    // Calculate lighting with normal
    float3 normal = normalize(input.Normal);
    output.Color = CalcLightingFull(input.Position, normal);
    
    // Transform texture coordinates by UV matrix
    float4 uv = float4(input.TexCoord, 0.0, 1.0);
    output.TexCoord = mul(uv, UVMat0).xy;
    
    return output;
}

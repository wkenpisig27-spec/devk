//==============================================================================
// vs_pnt0_ld_t0uvmat.hlsl - Static mesh shader with lighting and UV matrix
// Static props: basic ambient+diffuse only, no rim/spec.
//==============================================================================
#define CEL_ENABLE 0
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_PNT input)
{
    VS_OUTPUT output;
    
    // Transform position to clip space
    float4 worldPos = float4(input.Position, 1.0);
    output.Position = mul(worldPos, ViewProj);
    
    // Basic ambient+diffuse only
    float3 normal = normalize(input.Normal);
    output.Color = CalcLighting(normal);
    
    // Transform texture coordinates by UV matrix
    float4 uv = float4(input.TexCoord, 0.0, 1.0);
    output.TexCoord = mul(uv, UVMat0).xy;
    
    return output;
}

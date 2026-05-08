//==============================================================================
// vs_pndt0_ld_t0uvmat.hlsl - Static mesh with vertex color, lighting, and UV matrix
// Static props: basic ambient+diffuse only, no rim/spec.
//==============================================================================
#define CEL_ENABLE 0
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_PNDT input)
{
    VS_OUTPUT output;
    
    // Transform position to clip space
    float4 worldPos = float4(input.Position, 1.0);
    output.Position = mul(worldPos, ViewProj);
    
    // Basic ambient+diffuse modulated with vertex color
    float3 normal = normalize(input.Normal);
    float4 lighting = CalcLighting(normal);
    output.Color = input.Color * lighting;
    
    // Transform texture coordinates by UV matrix
    float4 uv = float4(input.TexCoord, 0.0, 1.0);
    output.TexCoord = mul(uv, UVMat0).xy;
    
    return output;
}

//==============================================================================
// vs_pnt0_ld.hlsl - Static mesh shader with lighting
// Position, Normal, TexCoord -> lit output
// Static props and indicators: use basic ambient+diffuse only.
// CEL/rim/spec are for character skinmesh shaders, not ground indicators.
//==============================================================================
#define CEL_ENABLE 0
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_PNT input)
{
    VS_OUTPUT output;
    
    // Transform position to clip space
    float4 worldPos = float4(input.Position, 1.0);
    output.Position = mul(worldPos, ViewProj);
    
    // Basic ambient+diffuse only — no rim or toon-spec.
    float3 normal = normalize(input.Normal);
    output.Color = CalcLighting(normal);
    
    // Pass through texture coordinates
    output.TexCoord = input.TexCoord;
    
    return output;
}

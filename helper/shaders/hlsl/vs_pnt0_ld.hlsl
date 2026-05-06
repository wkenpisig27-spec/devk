//==============================================================================
// vs_pnt0_ld.hlsl - Static mesh shader with lighting
// Position, Normal, TexCoord -> lit output
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
    output.Color   = CalcLighting(normal);
    output.Specular = CalcSpecular(normal);
    
    // Pass through texture coordinates
    output.TexCoord = input.TexCoord;
    
    return output;
}

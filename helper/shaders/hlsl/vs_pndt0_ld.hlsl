//==============================================================================
// vs_pndt0_ld.hlsl - Static mesh shader with vertex color and lighting
//==============================================================================
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_PNDT input)
{
    VS_OUTPUT output;
    
    // Transform position to clip space
    float4 worldPos = float4(input.Position, 1.0);
    output.Position = mul(worldPos, ViewProj);
    
    // Calculate lighting and modulate with vertex color
    float3 normal = normalize(input.Normal);
    float4 lighting = CalcLighting(normal);
    output.Color    = input.Color * lighting;
    output.Specular = CalcSpecular(normal);
    
    // Pass through texture coordinates
    output.TexCoord = input.TexCoord;
    
    return output;
}

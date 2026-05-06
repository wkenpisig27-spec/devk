//==============================================================================
// vs_pndt0_ld.hlsl - Static mesh shader with vertex color and lighting
// CEL_ENABLE disabled: static meshes use smooth Lambert.
//==============================================================================
#define CEL_ENABLE 0
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_PNDT input)
{
    VS_OUTPUT output;
    
    // Transform position to clip space
    float4 worldPos = float4(input.Position, 1.0);
    output.Position = mul(worldPos, ViewProj);
    
    // Calculate lighting and modulate with vertex color
    float3 normal = normalize(input.Normal);
    float4 lighting = CalcLightingFull(input.Position, normal);
    output.Color = input.Color * lighting;
    
    // Pass through texture coordinates
    output.TexCoord = input.TexCoord;
    
    return output;
}

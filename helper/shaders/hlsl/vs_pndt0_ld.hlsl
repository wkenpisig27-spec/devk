//==============================================================================
// vs_pndt0_ld.hlsl - Static mesh shader with vertex color and lighting
// Static props: soft 2-band cel + tint, no rim/spec.
//==============================================================================
#define ENV_CEL_ENABLE 1
#define CEL_ENABLE 0
#define TINT_ENABLE 1
#define SPEC_ENABLE 0
#define RIM_ENABLE 0
#include "common.hlsli"

VS_OUTPUT main(VS_INPUT_PNDT input)
{
    VS_OUTPUT output;
    
    float4 worldPos = float4(input.Position, 1.0);
    output.Position = mul(worldPos, ViewProj);
    
    float3 normal = normalize(input.Normal);
    float4 lighting = CalcLightingEnv(normal);
    output.Color = input.Color * lighting;
    
    output.TexCoord = input.TexCoord;
    
    return output;
}

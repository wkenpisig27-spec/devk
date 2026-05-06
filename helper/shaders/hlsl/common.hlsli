//==============================================================================
// common.hlsli - Shared constants and functions for PKO vertex shaders
//==============================================================================

// Constant Registers (set by engine in lwxRenderCtrVS.cpp)
// c0      = Base {1.0, 0.0, 0.0, 765.01}  - w is bone index multiplier
// c1-c4   = ViewProjection matrix (transposed)
// c5      = Light direction (in object space)
// c6      = Ambient color
// c7      = Diffuse color
// c8-c11  = Texture Stage 0 UV Matrix
// c12-c15 = Texture Stage 1 UV Matrix
// c16-c19 = Texture Stage 2 UV Matrix
// c20     = Specular params {intensity, power, wrap, unused}
// c21+    = Bone matrix palette (3 registers per bone, transposed rows)

// Base constants
float4 Base         : register(c0);   // {1.0, 0.0, 0.0, 765.01}

// View-Projection matrix (transposed, so multiply pos * matrix)
float4x4 ViewProj   : register(c1);

// Lighting
float4 LightDir     : register(c5);   // Direction to light (object space)
float4 Ambient      : register(c6);   // Ambient light color
float4 Diffuse      : register(c7);   // Diffuse light color

// UV Transform matrices
float4x4 UVMat0     : register(c8);
float4x4 UVMat1     : register(c12);
float4x4 UVMat2     : register(c16);

// c20: Specular lighting parameters
//   x = specular intensity  (default 0.35)
//   y = specular power/shininess (default 24.0)
//   z = half-lambert wrap amount (default 0.5, range [0..1])
//   w = rim light intensity (default 0.0, set > 0 to enable)
float4 SpecularParams : register(c20);

// Bone palette starts at c21, 3 registers per bone (rows 0,1,2 of 4x3 matrix)
#define BONE_PALETTE_START 21
float4 BonePalette[150] : register(c21);  // 50 bones * 3 rows = 150

//------------------------------------------------------------------------------
// Vertex Input Structures
//------------------------------------------------------------------------------

// Skinned mesh with 1 bone influence (pu4nt0)
struct VS_INPUT_SKIN1
{
    float3 Position     : POSITION;
    float4 BlendIndices : BLENDINDICES;  // D3DCOLOR: packed indices as 0-1 floats
    float3 Normal       : NORMAL;
    float2 TexCoord     : TEXCOORD0;
};

// Skinned mesh with 2 bone influences (pb1u4nt0)
struct VS_INPUT_SKIN2
{
    float3 Position     : POSITION;
    float1 BlendWeight  : BLENDWEIGHT;
    float4 BlendIndices : BLENDINDICES;
    float3 Normal       : NORMAL;
    float2 TexCoord     : TEXCOORD0;
};

// Skinned mesh with 3 bone influences (pb2u4nt0)
struct VS_INPUT_SKIN3
{
    float3 Position     : POSITION;
    float2 BlendWeight  : BLENDWEIGHT;
    float4 BlendIndices : BLENDINDICES;
    float3 Normal       : NORMAL;
    float2 TexCoord     : TEXCOORD0;
};

// Skinned mesh with 4 bone influences (pb3u4nt0)
struct VS_INPUT_SKIN4
{
    float3 Position     : POSITION;
    float3 BlendWeight  : BLENDWEIGHT;
    float4 BlendIndices : BLENDINDICES;
    float3 Normal       : NORMAL;
    float2 TexCoord     : TEXCOORD0;
};

// Static mesh with Position, Normal, TexCoord
struct VS_INPUT_PNT
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

// Static mesh with Position, Normal, Diffuse color, TexCoord
struct VS_INPUT_PNDT
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

//------------------------------------------------------------------------------
// Common Output Structure
//------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Position : POSITION;
    float4 Color    : COLOR0;   // Diffuse + ambient lighting
    float4 Specular : COLOR1;   // Blinn-Phong specular (added by SpecularEnable)
    float2 TexCoord : TEXCOORD0;
};

//------------------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------------------

// Convert D3DCOLOR blend index component to bone palette base index
// D3DCOLOR stores values as 0-1 floats. Multiply by 765.01 gives raw value (0-255).
// Each bone uses 3 registers, so divide by 3 to get bone index, then multiply by 3 for register base.
// Example: index 63 in D3DCOLOR (= 63/255) -> 63/255 * 765.01 ≈ 189 -> 189/3*3 = 189 (register for bone 63)
int GetBoneRegisterBase(float colorIndex)
{
    // colorIndex is 0-1, multiply by 765.01 to get raw (matches c0.w)
    // Then we need integer division by 3, multiply by 3
    // The original shaders do: index * 765.01, then use that directly since bones are at 3-register intervals
    // Wait - let me re-check. The original does mul by 765, no divide.
    // That means the packed value IS the register offset already.
    // So index 0,1,2 = bone 0; index 3,4,5 = bone 1, etc.
    return (int)(colorIndex * Base.w);
}

// Transform position by bone matrix (bone matrices are transposed rows in palette)
float4 TransformByBone(float3 pos, int boneBase)
{
    float4 p = float4(pos, 1.0);
    float4 result;
    result.x = dot(p, BonePalette[boneBase + 0]);
    result.y = dot(p, BonePalette[boneBase + 1]);
    result.z = dot(p, BonePalette[boneBase + 2]);
    result.w = 1.0;
    return result;
}

// Transform normal by bone matrix (no translation)
float3 TransformNormalByBone(float3 nrm, int boneBase)
{
    float3 result;
    result.x = dot(nrm, BonePalette[boneBase + 0].xyz);
    result.y = dot(nrm, BonePalette[boneBase + 1].xyz);
    result.z = dot(nrm, BonePalette[boneBase + 2].xyz);
    return result;
}

// Calculate directional light contribution with half-lambert wrap
// Half-lambert remaps the NdotL from the standard [-1..1] hard cutoff to a
// soft [0..1] wrap, giving surfaces a rounder, more 3D appearance.
// The wrap amount from SpecularParams.z controls the blend:
//   0.0 = standard Lambertian (sharp terminator)
//   0.5 = classic half-lambert (smooth, Valve-style)
float4 CalcLighting(float3 normal)
{
    float wrap  = SpecularParams.z;               // e.g. 0.5
    float NdotL = dot(normal, LightDir.xyz);

    // Wrap formula: remaps NdotL so backfaces still receive a fraction of diffuse
    NdotL = saturate((NdotL + wrap) / (1.0 + wrap));

    return Ambient + Diffuse * NdotL;
}

// Blinn-Phong specular using an approximate view direction suited for the
// PKO isometric camera (high angle, slightly forward-oriented).
// Intensity and power are driven by SpecularParams.x and SpecularParams.y.
float4 CalcSpecular(float3 normal)
{
    // Approximate normalized view direction for an isometric top-down camera.
    // Represents a camera sitting slightly above and forward of the scene.
    static const float3 kViewDir = float3(0.0, 0.4472, 0.8944); // normalize(0, 0.5, 1)

    float3 H     = normalize(LightDir.xyz + kViewDir);
    float  NdotH = saturate(dot(normal, H));
    float  spec  = pow(NdotH, SpecularParams.y) * SpecularParams.x;

    // Return specular as a tinted highlight using the diffuse light color; alpha 0
    return float4(Diffuse.rgb * spec, 0.0);
}

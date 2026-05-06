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

// Camera position in object space (for rim light / toon specular)
// Set by engine in lwxRenderCtrVS.cpp BeginSet().
float4 EyePosOS     : register(c20);

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
    float4 Color    : COLOR0;
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

//------------------------------------------------------------------------------
// Outline pass helpers
//------------------------------------------------------------------------------
// Outline width as a fraction of NDC (clip-space xy is [-1,1], so this
// translates to roughly OUTLINE_NDC * 0.5 * screen_width pixels).
//   0.003  -> ~2 px at 1280, ~3 px at 1920
//   0.0025 -> ~1.6 px at 1280, ~2.4 px at 1920 (default — modern toon look)
#ifndef OUTLINE_NDC
#define OUTLINE_NDC 0.0025
#endif

// Outline color. Pure black reads as "early-2000s toon"; dark-tinted reads as
// "modern stylized" (Genshin / Hi-Fi Rush). The FF MODULATE stage downstream
// multiplies this against the bound texture, so dark non-zero values yield a
// near-black outline with a subtle hue shift from the underlying material.
#ifndef OUTLINE_COLOR
#define OUTLINE_COLOR float4(0.10, 0.06, 0.08, 1.0)
#endif

// Distance-scaled outline extrusion: extrude in clip-space along the
// screen-projected normal, scaled by w so the perspective divide leaves a
// constant NDC offset → constant pixel width regardless of depth.
//
// posOS - object-space position (post-bone for skinned meshes)
// nrmOS - object-space normal (post-bone for skinned meshes)
float4 OutlineClipPos(float3 posOS, float3 nrmOS)
{
    float4 clipPos = mul(float4(posOS, 1.0), ViewProj);
    float4 clipNrm = mul(float4(nrmOS, 0.0), ViewProj);

    // Direction in screen space (clip-space xy projected to NDC).
    float2 dir = normalize(clipNrm.xy);

    clipPos.xy += dir * OUTLINE_NDC * clipPos.w;
    return clipPos;
}

// 2.5D toon shading pipeline:
//   1. Cel-quantized diffuse (existing)
//   2. Tinted shadow / warm highlight (hue shift across the band)
//   3. Toon specular (hard highlight blob on lit side)
//   4. Rim light (warm fresnel along silhouette)
//
// Each of (CEL/TINT/SPEC/RIM) has its own enable flag — toggle independently.
// All four output through output.Color, modulated against texture by the FF
// MODULATE stage. No pixel shader required.

// --- Cel bands -------------------------------------------------------------
#ifndef CEL_ENABLE
#define CEL_ENABLE   1
#endif
#define CEL_BAND0    0.40   // shadow -> mid threshold
#define CEL_BAND1    0.75   // mid    -> lit threshold
#define CEL_LEVEL0   0.45   // shadow brightness
#define CEL_LEVEL1   0.75   // mid brightness
#define CEL_LEVEL2   1.00   // lit brightness

// --- Shadow tint (cool shadow / warm highlight) ----------------------------
#ifndef TINT_ENABLE
#define TINT_ENABLE  1
#endif
// Subtle hue shift; multiply the lit color so it survives ambient changes.
#define SHADOW_TINT  float3(0.92, 0.96, 1.05)   // gentle cool
#define LIGHT_TINT   float3(1.04, 1.00, 0.95)   // gentle warm

// --- Toon specular (hard blob highlight) -----------------------------------
#ifndef SPEC_ENABLE
#define SPEC_ENABLE  1
#endif
#define SPEC_THRESH  0.93    // dot(N,H) above this = highlight (sharp blob)
#define SPEC_GAIN    0.70    // additive intensity
#define SPEC_COLOR   float3(1.00, 0.95, 0.85)

// --- Rim light (warm fresnel) ----------------------------------------------
#ifndef RIM_ENABLE
#define RIM_ENABLE   1
#endif
#define RIM_THRESH   0.45    // 1 - dot(N,V) above this = rim
#define RIM_GAIN     0.85    // additive intensity (raised so it survives saturation)
#define RIM_COLOR    float3(1.00, 0.85, 0.65)   // warmer for stronger anime read

// Compute base diffuse band (cel or half-Lambert), with optional tint.
float4 _ApplyDiffuseBand(float NdotL)
{
#if CEL_ENABLE
    // Quantize into 3 bands using step() (vs_1_1 friendly, no flow control).
    float band = CEL_LEVEL0
               + step(CEL_BAND0, NdotL) * (CEL_LEVEL1 - CEL_LEVEL0)
               + step(CEL_BAND1, NdotL) * (CEL_LEVEL2 - CEL_LEVEL1);
#else
    // Half-Lambert keeps flat/upward surfaces from going black.
    float band = NdotL * 0.5 + 0.5;
#endif

    float3 lit = Ambient.rgb + Diffuse.rgb * band;

#if TINT_ENABLE
    lit *= lerp(SHADOW_TINT, LIGHT_TINT, band);
#endif

    return float4(lit, Ambient.a);
}

// Position-aware lighting: cel + tint + toon-spec + rim.
// posOS  - vertex position in object space (skinned: post-bone; static: input.Position)
// normal - normalized object-space normal
float4 CalcLightingFull(float3 posOS, float3 normal)
{
    float NdotL = max(0, dot(normal, LightDir.xyz));
    float4 lit  = _ApplyDiffuseBand(NdotL);

#if (RIM_ENABLE || SPEC_ENABLE)
    float3 V = normalize(EyePosOS.xyz - posOS);
#endif

#if SPEC_ENABLE
    // Half-vector toon specular. step() gives a hard-edged highlight.
    // Mask by lit side so the highlight cannot bleed into the shadow band.
    float3 H      = normalize(V + LightDir.xyz);
    float NdotH   = max(0, dot(normal, H));
    float specMsk = step(SPEC_THRESH, NdotH) * step(CEL_BAND1, NdotL);
    lit.rgb += SPEC_COLOR * (specMsk * SPEC_GAIN);
#endif

#if RIM_ENABLE
    // Fresnel rim — bright halo on grazing angles, regardless of NdotL.
    float fresnel = 1.0 - max(0, dot(normal, V));
    float rimMsk  = step(RIM_THRESH, fresnel);
    lit.rgb += RIM_COLOR * (rimMsk * RIM_GAIN);
#endif

    return lit;
}

// Backward-compatible normal-only entry point (no rim / no spec).
// Use when caller has no convenient object-space position.
float4 CalcLighting(float3 normal)
{
    float NdotL = max(0, dot(normal, LightDir.xyz));
    return _ApplyDiffuseBand(NdotL);
}

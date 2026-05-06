// ============================================================
// shadow_receive.fx - PKO Engine Shadow Map Overlay
// ============================================================
// Renders a full-screen-quad-like overlay on the terrain to apply
// directional light shadows from a shadow map texture.
//
// Usage:
//   1. Bind shadow map texture via g_texShadowMap
//   2. Set g_matShadowTransform (world -> shadow UV)
//   3. Set g_matWorldViewProj (world -> screen)
//   4. Render terrain geometry with this effect active
// ============================================================

float4x4 g_matWorld;
float4x4 g_matWorldViewProj;
float4x4 g_matShadowTransform;
float    g_fShadowIntensity = 0.5;
float    g_fDepthBias = 0.0005;
float    g_fTexelSize = 0.0009766;  // Set from C++: 1.0 / shadow map resolution
float    g_fBorderFade = 0.10;     // Fraction of projection edge over which shadow fades to 0
float    g_fPCFRadius  = 3.0;      // Poisson kernel radius in texels

// Sky-ambient tint for shadow areas.
// Instead of pure black (0,0,0), shadows are blended toward this color,
// simulating indirect skylight reaching occluded surfaces.
// Default: a dark, cool blue-grey that reads as outdoor indirect sky light.
// RGB should be very dark - it blends as: tint * shadow + scene * (1-shadow).
float4   g_vShadowTint = float4(0.06, 0.09, 0.16, 1.0);

texture  g_texShadowMap;

sampler ShadowMapSampler = sampler_state {
    Texture   = <g_texShadowMap>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU  = BORDER;
    AddressV  = BORDER;
    BorderColor = float4(1, 1, 1, 1);
};

// ============================================================
// Shadow Overlay Pass
// Renders shadow darkness on top of the existing scene
// ============================================================
struct VS_INPUT {
    float4 Pos : POSITION;
};

struct VS_OUTPUT {
    float4 Pos       : POSITION;
    float4 ShadowUV  : TEXCOORD0;
};

VS_OUTPUT VS_ShadowOverlay(VS_INPUT In) {
    VS_OUTPUT Out;
    float4 worldPos = mul(In.Pos, g_matWorld);
    Out.Pos = mul(In.Pos, g_matWorldViewProj);
    Out.ShadowUV = mul(worldPos, g_matShadowTransform);
    return Out;
}

float4 PS_ShadowOverlay(VS_OUTPUT In) : COLOR0 {
    float2 shadowUV = In.ShadowUV.xy / In.ShadowUV.w;
    float  pixelDepth = In.ShadowUV.z / In.ShadowUV.w;
    
    // Check if pixel is outside shadow map bounds
    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0)
        return float4(0, 0, 0, 0);
    
    float shadowMapDepth = tex2D(ShadowMapSampler, shadowUV).r;
    
    // Shadow test with bias
    float shadow = (pixelDepth - g_fDepthBias > shadowMapDepth) ? g_fShadowIntensity : 0.0;
    
    return float4(0, 0, 0, shadow);
}

// PCF (Percentage Closer Filtering) version for softer shadows 
float4 PS_ShadowOverlayPCF(VS_OUTPUT In) : COLOR0 {
    float2 shadowUV = In.ShadowUV.xy / In.ShadowUV.w;
    float  pixelDepth = In.ShadowUV.z / In.ShadowUV.w;
    
    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0)
        return float4(0, 0, 0, 0);
    
    // 4-tap PCF for slightly softer shadow edges
    float texelSize = g_fTexelSize;
    float shadow = 0.0;
    
    float2 offsets[4] = {
        float2(-0.5, -0.5),
        float2( 0.5, -0.5),
        float2(-0.5,  0.5),
        float2( 0.5,  0.5)
    };
    
    for (int i = 0; i < 4; i++) {
        float2 uv = shadowUV + offsets[i] * texelSize;
        float depth = tex2D(ShadowMapSampler, uv).r;
        shadow += (pixelDepth - g_fDepthBias > depth) ? 1.0 : 0.0;
    }
    
    shadow = (shadow / 4.0) * g_fShadowIntensity;
    return float4(0, 0, 0, shadow);
}

// ============================================================
// ShadowReceive — production technique
// 12-tap Poisson PCF for smooth silhouette edges combined with
// a smoothstep border fade so the shadow dissolves toward the
// edge of the projection area (like a blob shadow gradient).
// ============================================================
float4 PS_ShadowReceive(VS_OUTPUT In) : COLOR0 {
    float2 shadowUV  = In.ShadowUV.xy / In.ShadowUV.w;
    float  pixelDepth = In.ShadowUV.z  / In.ShadowUV.w;

    // Smooth fade near each edge of the shadow projection.
    // edgeDist.x/y = distance from the nearest U or V boundary [0,1].
    // Below g_fBorderFade the shadow linearly dissolves to transparent.
    float2 edgeDist  = min(shadowUV, 1.0 - shadowUV);
    float borderFade = smoothstep(0.0, g_fBorderFade, edgeDist.x)
                     * smoothstep(0.0, g_fBorderFade, edgeDist.y);
    if (borderFade <= 0.001)
        return float4(0, 0, 0, 0);

    // 12-tap Poisson disk PCF — distributes samples irregularly so the
    // soft edge has no visible grid pattern.
    static const float2 poissonDisk[12] = {
        float2(-0.3262, -0.4058),
        float2(-0.8401, -0.0736),
        float2(-0.6959,  0.4571),
        float2(-0.2033,  0.6207),
        float2( 0.9623, -0.1950),
        float2( 0.4734, -0.4800),
        float2( 0.5195,  0.7670),
        float2( 0.1855, -0.8931),
        float2( 0.5074,  0.0644),
        float2( 0.8964,  0.4125),
        float2(-0.3219, -0.9326),
        float2(-0.7916, -0.5977)
    };

    float radius = g_fTexelSize * g_fPCFRadius;
    float shadow = 0.0;
    [unroll] for (int i = 0; i < 12; i++) {
        float2 uv  = shadowUV + poissonDisk[i] * radius;
        float  dep = tex2D(ShadowMapSampler, uv).r;
        shadow += (pixelDepth - g_fDepthBias > dep) ? 1.0 : 0.0;
    }
    shadow = (shadow / 12.0) * g_fShadowIntensity * borderFade;

    // Blend toward g_vShadowTint instead of pure black.
    // This simulates indirect skylight in shadow areas (cool blue-grey by default).
    // Blend equation: tint * shadow + sceneColor * (1-shadow)
    return float4(g_vShadowTint.rgb, shadow);
}

technique ShadowReceive {
    pass P0 {
        VertexShader = compile vs_3_0 VS_ShadowOverlay();
        PixelShader  = compile ps_3_0 PS_ShadowReceive();

        ZEnable          = TRUE;
        ZWriteEnable     = FALSE;
        AlphaBlendEnable = TRUE;
        SrcBlend         = SRCALPHA;
        DestBlend        = INVSRCALPHA;
        CullMode         = NONE;
    }
}

technique ShadowOverlay {
    pass P0 {
        VertexShader = compile vs_2_0 VS_ShadowOverlay();
        PixelShader  = compile ps_2_0 PS_ShadowOverlay();
        
        ZEnable          = TRUE;
        ZWriteEnable     = FALSE;
        AlphaBlendEnable = TRUE;
        SrcBlend         = SRCALPHA;
        DestBlend        = INVSRCALPHA;
        CullMode         = NONE;
    }
}

technique ShadowOverlayPCF {
    pass P0 {
        VertexShader = compile vs_2_0 VS_ShadowOverlay();
        PixelShader  = compile ps_2_0 PS_ShadowOverlayPCF();
        
        ZEnable          = TRUE;
        ZWriteEnable     = FALSE;
        AlphaBlendEnable = TRUE;
        SrcBlend         = SRCALPHA;
        DestBlend        = INVSRCALPHA;
        CullMode         = NONE;
    }
}

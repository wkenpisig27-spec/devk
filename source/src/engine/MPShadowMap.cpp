#include "StdAfx.h"
#include "GlobalInc.h"
#include "MPShadowMap.h"
#include "MPRender.h"
#include "lwDeviceObject.h"

extern MINDPOWER_API MPRender g_Render;

// Shadow effect HLSL source embedded as string
// This keeps the shadow system self-contained without requiring external .fx files at startup
static const char* s_szShadowEffectSource = R"HLSL(
// ============================================================
// Shadow Map Effect - PKO Engine
// ============================================================
// Renders scene depth from the light's point of view (shadow pass)
// and applies shadow lookup during the main scene pass.
// ============================================================

float4x4 g_matLightViewProj;
float4x4 g_matWorld;
float4x4 g_matShadowTransform;  // Light VP * texture scale/bias
float4x4 g_matWorldViewProj;
float    g_fShadowIntensity;
float    g_fDepthBias;
float    g_fTexelSize;     // 1.0 / shadow map resolution
float    g_fBorderFade;    // Fraction of UV space for edge fade (e.g. 0.10)
float    g_fPCFRadius;     // Poisson kernel radius in texels (e.g. 3.0)

texture  g_texShadowMap;

sampler ShadowSampler = sampler_state {
    Texture   = <g_texShadowMap>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU  = BORDER;
    AddressV  = BORDER;
    BorderColor = 0xFFFFFFFF;
};

// ============================================================
// Technique 0: Shadow Depth Pass
// Renders depth from the light's perspective into the shadow map
// ============================================================
struct VS_DEPTH_INPUT {
    float4 Pos : POSITION;
};

struct VS_DEPTH_OUTPUT {
    float4 Pos   : POSITION;
    float2 Depth : TEXCOORD0;  // x = depth, y = depth (for MRT-less cards)
};

VS_DEPTH_OUTPUT VS_ShadowDepth(VS_DEPTH_INPUT In) {
    VS_DEPTH_OUTPUT Out;
    float4 worldPos = mul(In.Pos, g_matWorld);
    Out.Pos = mul(worldPos, g_matLightViewProj);
    Out.Depth.x = Out.Pos.z;
    Out.Depth.y = Out.Pos.w;
    return Out;
}

float4 PS_ShadowDepth(VS_DEPTH_OUTPUT In) : COLOR0 {
    float depth = In.Depth.x / In.Depth.y;
    return float4(depth, depth, depth, 1.0f);
}

technique ShadowDepth {
    pass P0 {
        VertexShader = compile vs_2_0 VS_ShadowDepth();
        PixelShader  = compile ps_2_0 PS_ShadowDepth();
        
        CullMode = CCW;
        ZEnable  = TRUE;
        ZWriteEnable = TRUE;
        AlphaBlendEnable = FALSE;
        ColorWriteEnable = RED | GREEN | BLUE | ALPHA;
    }
}

// ============================================================
// Technique 1: Shadow Depth Pass for Skinned Meshes
// (same as above but with world matrix baked in)
// ============================================================
technique ShadowDepthSkinned {
    pass P0 {
        VertexShader = compile vs_2_0 VS_ShadowDepth();
        PixelShader  = compile ps_2_0 PS_ShadowDepth();
        
        CullMode = CCW;
        ZEnable  = TRUE;
        ZWriteEnable = TRUE;
        AlphaBlendEnable = FALSE;
    }
}

// ============================================================
// Technique 2: Shadow Receive (Apply shadow to terrain/objects)
// Overlays shadow darkness onto already-rendered scene geometry
// ============================================================
struct VS_RECEIVE_INPUT {
    float4 Pos : POSITION;
};

struct VS_RECEIVE_OUTPUT {
    float4 Pos       : POSITION;
    float4 ShadowUV  : TEXCOORD0;
};

VS_RECEIVE_OUTPUT VS_ShadowReceive(VS_RECEIVE_INPUT In) {
    VS_RECEIVE_OUTPUT Out;
    float4 worldPos = mul(In.Pos, g_matWorld);
    Out.Pos = mul(worldPos, g_matWorldViewProj);
    Out.ShadowUV = mul(worldPos, g_matShadowTransform);
    return Out;
}

float4 PS_ShadowReceive(VS_RECEIVE_OUTPUT In) : COLOR0 {
    // Project shadow UV
    float2 shadowUV = In.ShadowUV.xy / In.ShadowUV.w;

    // Edge fade: smoothly dissolve the shadow near the projection boundary
    // so it doesn't hard-cut when shadow casters leave the frustum.
    // edgeDist ranges 0 (at UV edge) to 1 (past the fade zone toward center).
    float2 edgeDist = saturate((0.5 - abs(shadowUV - 0.5)) / g_fBorderFade);
    float fade = edgeDist.x * edgeDist.y;

    // 12-tap Poisson disk PCF using hardware bilinear filtering.
    // IMPORTANT: do NOT use a binary threshold (< 0.99 ? 1 : 0).
    // Instead use (1.0 - sample.r) directly so the GPU's LINEAR sampler
    // contributes a smooth 0..1 value at shadow edges — this eliminates
    // the stepped/pixelated silhouette that binary thresholding causes.
    // shadow texels = black (r≈0) → contribution 1.0
    // lit texels    = white (r≈1) → contribution 0.0
    // bilinear edge = smoothly interpolated → smooth penumbra
    float r = g_fPCFRadius * g_fTexelSize;
    float total = 0.0;
    total += (1.0 - tex2D(ShadowSampler, shadowUV + float2(-0.326*r, -0.406*r)).r);
    total += (1.0 - tex2D(ShadowSampler, shadowUV + float2(-0.840*r, -0.074*r)).r);
    total += (1.0 - tex2D(ShadowSampler, shadowUV + float2(-0.696*r,  0.457*r)).r);
    total += (1.0 - tex2D(ShadowSampler, shadowUV + float2(-0.203*r,  0.621*r)).r);
    total += (1.0 - tex2D(ShadowSampler, shadowUV + float2( 0.962*r, -0.195*r)).r);
    total += (1.0 - tex2D(ShadowSampler, shadowUV + float2( 0.473*r, -0.480*r)).r);
    total += (1.0 - tex2D(ShadowSampler, shadowUV + float2( 0.519*r,  0.767*r)).r);
    total += (1.0 - tex2D(ShadowSampler, shadowUV + float2( 0.185*r, -0.893*r)).r);
    total += (1.0 - tex2D(ShadowSampler, shadowUV + float2( 0.507*r,  0.064*r)).r);
    total += (1.0 - tex2D(ShadowSampler, shadowUV + float2( 0.896*r,  0.412*r)).r);
    total += (1.0 - tex2D(ShadowSampler, shadowUV + float2(-0.321*r, -0.933*r)).r);
    total += (1.0 - tex2D(ShadowSampler, shadowUV + float2(-0.791*r,  0.498*r)).r);

    float shadow = (total / 12.0) * g_fShadowIntensity * fade;

    // Subtle blue-gray tint instead of pure black — simulates sky ambient bounce
    // light filling shadowed areas in an outdoor scene.
    return float4(0.05, 0.06, 0.12, shadow);
}

technique ShadowReceive {
    pass P0 {
        VertexShader = compile vs_2_0 VS_ShadowReceive();
        PixelShader  = compile ps_2_0 PS_ShadowReceive();

        ZEnable          = FALSE;
        ZWriteEnable     = FALSE;
        AlphaBlendEnable = TRUE;
        SrcBlend         = SRCALPHA;
        DestBlend        = INVSRCALPHA;
        CullMode         = NONE;
    }
}
)HLSL";

// ============================================================
// CMPShadowMap Implementation
// ============================================================

CMPShadowMap::CMPShadowMap()
    : _pDev(nullptr)
    , _bInitialized(false)
    , _pShadowTexture(nullptr)
    , _pShadowSurface(nullptr)
    , _pShadowDepthSurface(nullptr)
    , _pOldRenderTarget(nullptr)
    , _pOldDepthSurface(nullptr)
    , _pShadowEffect(nullptr)
    , _vLightDir(0.0f, 0.0f, -1.0f)
    , _vFocusPoint(0.0f, 0.0f, 0.0f)
    , _vFocusPointSmoothed(0.0f, 0.0f, 0.0f)
    , _bFocusInitialized(false)
    , _pGroundVB(nullptr)
    , _pGroundIB(nullptr)
    , _nGroundGridSize(32)
    , _nGroundVertCount(0)
    , _nGroundTriCount(0)
{
    D3DXMatrixIdentity(&_matLightView);
    D3DXMatrixIdentity(&_matLightProj);
    D3DXMatrixIdentity(&_matLightViewProj);
    D3DXMatrixIdentity(&_matShadowTransform);
    memset(&_oldViewport, 0, sizeof(_oldViewport));
}

CMPShadowMap::~CMPShadowMap() {
    Release();
}

bool CMPShadowMap::Create(IDirect3DDeviceX* pDev, const ShadowMapConfig& config) {
    if (!pDev)
        return false;

    _pDev = pDev;
    _config = config;

    // Check device capabilities
    D3DCAPS9 caps;
    _pDev->GetDeviceCaps(&caps);

    // Need at least VS 2.0 and PS 2.0 for shadow mapping
    if (caps.VertexShaderVersion < D3DVS_VERSION(2, 0) ||
        caps.PixelShaderVersion < D3DPS_VERSION(2, 0)) {
        LG("shadow", "Shadow mapping requires VS/PS 2.0. Disabling.\n");
        _config.enabled = false;
        return false;
    }

    // Compile the shadow effect
    LPD3DXBUFFER pErrors = nullptr;
    HRESULT hr = D3DXCreateEffect(
        _pDev,
        s_szShadowEffectSource,
        (UINT)strlen(s_szShadowEffectSource),
        nullptr,  // defines
        nullptr,  // include
        D3DXFX_DONOTSAVESHADERSTATE,
        nullptr,  // pool
        &_pShadowEffect,
        &pErrors
    );

    if (FAILED(hr)) {
        if (pErrors) {
            LG("shadow", "Shadow effect compile error: %s\n", (const char*)pErrors->GetBufferPointer());
            pErrors->Release();
        }
        else {
            LG("shadow", "Shadow effect compile failed: 0x%08X\n", hr);
        }
        _config.enabled = false;
        return false;
    }
    if (pErrors) pErrors->Release();

    if (!CreateResources()) {
        Release();
        return false;
    }

    _bInitialized = true;
    LG("shadow", "Shadow map created: %dx%d\n", _config.resolution, _config.resolution);
    return true;
}

bool CMPShadowMap::CreateResources() {
    int res = _config.resolution;

    // Create shadow map as a renderable texture
    // Use A8R8G8B8 since we detect character silhouettes by color (need all RGB channels)
    D3DFORMAT texFmt = D3DFMT_A8R8G8B8;
    HRESULT hr = _pDev->CreateTexture(
        res, res, 1,
        D3DUSAGE_RENDERTARGET,
        texFmt,
        D3DPOOL_DEFAULT,
        &_pShadowTexture,
        nullptr
    );

    if (FAILED(hr)) {
        LG("shadow", "Failed to create shadow map texture: 0x%08X\n", hr);
        return false;
    }

    // Get the surface from the texture for use as render target
    hr = _pShadowTexture->GetSurfaceLevel(0, &_pShadowSurface);
    if (FAILED(hr)) {
        LG("shadow", "Failed to get shadow surface: 0x%08X\n", hr);
        return false;
    }

    // Create depth/stencil surface for shadow rendering
    hr = _pDev->CreateDepthStencilSurface(
        res, res,
        D3DFMT_D24S8,
        D3DMULTISAMPLE_NONE,
        0,
        TRUE,  // discard
        &_pShadowDepthSurface,
        nullptr
    );

    if (FAILED(hr)) {
        // Try D3DFMT_D16 as fallback
        hr = _pDev->CreateDepthStencilSurface(
            res, res,
            D3DFMT_D16,
            D3DMULTISAMPLE_NONE,
            0,
            TRUE,
            &_pShadowDepthSurface,
            nullptr
        );
        if (FAILED(hr)) {
            LG("shadow", "Failed to create shadow depth surface: 0x%08X\n", hr);
            return false;
        }
    }

    BuildGroundGrid();

    return true;
}

void CMPShadowMap::ReleaseResources() {
    if (_pShadowSurface) { _pShadowSurface->Release(); _pShadowSurface = nullptr; }
    if (_pShadowDepthSurface) { _pShadowDepthSurface->Release(); _pShadowDepthSurface = nullptr; }
    if (_pShadowTexture) { _pShadowTexture->Release(); _pShadowTexture = nullptr; }
    if (_pGroundVB) { _pGroundVB->Release(); _pGroundVB = nullptr; }
    if (_pGroundIB) { _pGroundIB->Release(); _pGroundIB = nullptr; }
}

void CMPShadowMap::Release() {
    ReleaseResources();
    if (_pShadowEffect) { _pShadowEffect->Release(); _pShadowEffect = nullptr; }
    _bInitialized = false;
    _pDev = nullptr;
}

void CMPShadowMap::OnLostDevice() {
    ReleaseResources();
    if (_pShadowEffect)
        _pShadowEffect->OnLostDevice();
}

bool CMPShadowMap::OnResetDevice() {
    if (_pShadowEffect)
        _pShadowEffect->OnResetDevice();
    return CreateResources();
}

void CMPShadowMap::SetConfig(const ShadowMapConfig& config) {
    bool needRecreate = (_config.resolution != config.resolution);
    _config = config;

    if (needRecreate && _bInitialized) {
        ReleaseResources();
        if (!CreateResources()) {
            _config.enabled = false;
            LG("shadow", "Failed to recreate shadow map with new resolution\n");
        }
    }
}

void CMPShadowMap::SetLightDirection(const D3DXVECTOR3& dir) {
    D3DXVec3Normalize(&_vLightDir, &dir);
}

void CMPShadowMap::SetLightDirection(float x, float y, float z) {
    _vLightDir = D3DXVECTOR3(x, y, z);
    D3DXVec3Normalize(&_vLightDir, &_vLightDir);
}

void CMPShadowMap::SetFocusPoint(const D3DXVECTOR3& pos) {
    _vFocusPoint = pos;
}

void CMPShadowMap::UpdateLightMatrices() {
    // Smooth the working focus point toward the target so terrain-height
    // transitions move the shadow gradually instead of snapping in one frame.
    // On the very first call, snap immediately to avoid an initial slide-in.
    if (!_bFocusInitialized) {
        _vFocusPointSmoothed = _vFocusPoint;
        _bFocusInitialized   = true;
    } else {
        // XY tracks position quickly (0.25/frame) so the shadow stays
        // close under the character at normal walking speeds.
        // Z (height) uses a gentler rate (0.12/frame) to absorb the
        // abrupt step when crossing a terrain-height boundary.
        const float kXY = 0.25f;
        const float kZ  = 0.12f;
        _vFocusPointSmoothed.x += (_vFocusPoint.x - _vFocusPointSmoothed.x) * kXY;
        _vFocusPointSmoothed.y += (_vFocusPoint.y - _vFocusPointSmoothed.y) * kXY;
        _vFocusPointSmoothed.z += (_vFocusPoint.z - _vFocusPointSmoothed.z) * kZ;
    }

    // Steepen the light direction so shadows stay close to character size.
    // The map's light direction often has a low sun angle which creates
    // very elongated shadows. We reduce the horizontal (XY) components
    // to make the shadow projection more overhead while keeping direction.
    D3DXVECTOR3 shadowDir = _vLightDir;
    shadowDir.x *= 0.35f;
    shadowDir.y *= 0.35f;
    D3DXVec3Normalize(&shadowDir, &shadowDir);

    // Light position: move back from focus along opposite light direction
    D3DXVECTOR3 lightPos = _vFocusPointSmoothed - shadowDir * _config.lightHeight;

    // Build light view matrix looking at the focus point
    // Use an appropriate up vector (avoid degenerate case when light is straight down)
    D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
    if (fabsf(D3DXVec3Dot(&shadowDir, &up)) > 0.99f) {
        up = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
    }

    D3DXMatrixLookAtLH(&_matLightView, &lightPos, &_vFocusPointSmoothed, &up);

    // Orthographic projection for directional light shadows
    float halfSize = _config.orthoSize;
    D3DXMatrixOrthoLH(&_matLightProj,
        halfSize * 2.0f,  // width
        halfSize * 2.0f,  // height
        _config.nearPlane,
        _config.farPlane
    );

    // Combined light view-projection
    D3DXMatrixMultiply(&_matLightViewProj, &_matLightView, &_matLightProj);

    // Shadow transform: light VP * scale/bias to convert from clip space [-1,1] to UV [0,1]
    D3DXMATRIX matTexScale(
        0.5f,  0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f,  0.0f, 1.0f, 0.0f,
        0.5f,  0.5f, 0.0f, 1.0f
    );

    D3DXMatrixMultiply(&_matShadowTransform, &_matLightViewProj, &matTexScale);
}

bool CMPShadowMap::BeginShadowPass() {
    if (!IsEnabled() || !_pShadowSurface || !_pShadowDepthSurface)
        return false;

    // Update light matrices before rendering
    UpdateLightMatrices();

    // Save current render targets.
    // Release any previously saved surfaces to guard against a missed EndShadowPass
    // (e.g. skipped frame, early-return path) that would orphan the COM references.
    if (_pOldRenderTarget) { _pOldRenderTarget->Release(); _pOldRenderTarget = nullptr; }
    if (_pOldDepthSurface)  { _pOldDepthSurface->Release();  _pOldDepthSurface = nullptr; }
    _pDev->GetRenderTarget(0, &_pOldRenderTarget);
    _pDev->GetDepthStencilSurface(&_pOldDepthSurface);
    _pDev->GetViewport(&_oldViewport);

    // Save current View/Proj matrices from the engine device object
    MPIDeviceObject* pDevObj = g_Render.GetInterfaceMgr()->dev_obj;
    _matSavedView = *(D3DXMATRIX*)pDevObj->GetMatView();
    _matSavedProj = *(D3DXMATRIX*)pDevObj->GetMatProj();

    // Set light View/Proj so character rendering uses light-space transforms
    // This updates both the D3D device state AND the cached _mat_viewproj
    // which vertex shader render controllers read from
    g_Render.SetTransformView(&_matLightView);
    g_Render.SetTransformProj(&_matLightProj);

    // Set shadow map as render target
    HRESULT hr = _pDev->SetRenderTarget(0, _pShadowSurface);
    if (FAILED(hr)) {
        LG("shadow", "Failed to set shadow render target: 0x%08X\n", hr);
        g_Render.SetTransformView(&_matSavedView);
        g_Render.SetTransformProj(&_matSavedProj);
        if (_pOldRenderTarget) { _pOldRenderTarget->Release(); _pOldRenderTarget = nullptr; }
        if (_pOldDepthSurface) { _pOldDepthSurface->Release(); _pOldDepthSurface = nullptr; }
        return false;
    }

    _pDev->SetDepthStencilSurface(_pShadowDepthSurface);

    // Set viewport to shadow map size
    D3DVIEWPORTX vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = (DWORD)_config.resolution;
    vp.Height = (DWORD)_config.resolution;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    _pDev->SetViewport(&vp);

    // Clear shadow map to white (no shadow). Characters will render their
    // colored pixels from the light's POV; any non-white pixel = shadow.
    _pDev->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0);

    // Force alpha test off and cull mode to NONE through the device object
    // (updates both the D3D device AND the state cache) BEFORE enabling shadow
    // pass mode, so the states are properly set. Then enable shadow pass mode 
    // to block materials from overriding these states in their BeginSet() calls.
    lwDeviceObject* pDevObj2 = static_cast<lwDeviceObject*>(g_Render.GetInterfaceMgr()->dev_obj);
    // Save ALPHAREF so EndShadowPass can restore the exact pre-pass value.
    _pDev->GetRenderState(D3DRS_ALPHAREF, &_savedAlphaRef);
    pDevObj2->SetRenderStateForced(D3DRS_ALPHATESTENABLE, FALSE);
    pDevObj2->SetRenderStateForced(D3DRS_ALPHABLENDENABLE, FALSE);
    pDevObj2->SetRenderStateForced(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
    pDevObj2->SetRenderStateForced(D3DRS_ALPHAREF, 0);
    pDevObj2->SetRenderStateForced(D3DRS_CULLMODE, D3DCULL_NONE);
    pDevObj2->SetShadowPassMode(true);

    // Force ALL character output to pure black by overriding texture color/alpha
    // operations via the raw D3D device. This makes the shadow detection reliable
    // regardless of what vertex/pixel shaders or materials the characters use.
    // Any non-white pixel on the shadow map = shadow.
    _pDev->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFF000000);  // Opaque black
    _pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    _pDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
    _pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    _pDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
    _pDev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    _pDev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    // Null out any pixel shader so the fixed-function TFACTOR pipeline is used.
    // Custom pixel shaders would ignore our texture stage overrides entirely.
    _pDev->SetPixelShader(NULL);

    return true;
}

void CMPShadowMap::SetAlphaCutoutCasterMode(bool enabled) {
    if (!_pDev)
        return;

    lwDeviceObject* pDevObj = static_cast<lwDeviceObject*>(g_Render.GetInterfaceMgr()->dev_obj);
    if (!pDevObj->IsShadowPassMode())
        return;  // Only valid inside BeginShadowPass/EndShadowPass

    if (enabled) {
        // Preserve texture alpha for alpha-tested foliage while still forcing RGB to black.
        pDevObj->SetRenderStateForced(D3DRS_ALPHATESTENABLE, TRUE);
        pDevObj->SetRenderStateForced(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
        pDevObj->SetRenderStateForced(D3DRS_ALPHAREF, 128);
        pDevObj->SetRenderStateForced(D3DRS_ALPHABLENDENABLE, FALSE);
        pDevObj->SetRenderStateForced(D3DRS_CULLMODE, D3DCULL_NONE);

        pDevObj->SetTextureStageStateForced(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        pDevObj->SetTextureStageStateForced(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
        pDevObj->SetTextureStageStateForced(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        pDevObj->SetTextureStageStateForced(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        pDevObj->SetTextureStageStateForced(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        pDevObj->SetTextureStageStateForced(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    } else {
        // Restore the solid silhouette mode used for characters.
        pDevObj->SetRenderStateForced(D3DRS_ALPHATESTENABLE, FALSE);
        pDevObj->SetRenderStateForced(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
        pDevObj->SetRenderStateForced(D3DRS_ALPHAREF, 0);
        pDevObj->SetRenderStateForced(D3DRS_ALPHABLENDENABLE, FALSE);
        pDevObj->SetRenderStateForced(D3DRS_CULLMODE, D3DCULL_NONE);

        pDevObj->SetTextureStageStateForced(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        pDevObj->SetTextureStageStateForced(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
        pDevObj->SetTextureStageStateForced(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        pDevObj->SetTextureStageStateForced(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
        pDevObj->SetTextureStageStateForced(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        pDevObj->SetTextureStageStateForced(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }

    _pDev->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFF000000);
    _pDev->SetPixelShader(NULL);
}

void CMPShadowMap::EndShadowPass() {
    // Restore original render targets
    if (_pOldRenderTarget) {
        _pDev->SetRenderTarget(0, _pOldRenderTarget);
        _pOldRenderTarget->Release();
        _pOldRenderTarget = nullptr;
    }

    if (_pOldDepthSurface) {
        _pDev->SetDepthStencilSurface(_pOldDepthSurface);
        _pOldDepthSurface->Release();
        _pOldDepthSurface = nullptr;
    }

    // Restore viewport
    _pDev->SetViewport(&_oldViewport);

    // Restore render states changed during shadow pass
    lwDeviceObject* pDevObj3 = static_cast<lwDeviceObject*>(g_Render.GetInterfaceMgr()->dev_obj);
    pDevObj3->SetShadowPassMode(false);
    pDevObj3->SetRenderStateForced(D3DRS_ALPHATESTENABLE, TRUE);
    pDevObj3->SetRenderStateForced(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    pDevObj3->SetRenderStateForced(D3DRS_ALPHAREF, _savedAlphaRef);
    pDevObj3->SetRenderStateForced(D3DRS_ALPHABLENDENABLE, FALSE);
    pDevObj3->SetRenderStateForced(D3DRS_SRCBLEND, D3DBLEND_ONE);
    pDevObj3->SetRenderStateForced(D3DRS_DESTBLEND, D3DBLEND_ZERO);
    pDevObj3->SetRenderStateForced(D3DRS_CULLMODE, D3DCULL_CCW);

    // Restore texture stage states that BeginShadowPass overrode.
    // Reset to standard modulate pipeline (texture * diffuse).
    pDevObj3->SetTextureStageStateForced(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    pDevObj3->SetTextureStageStateForced(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    pDevObj3->SetTextureStageStateForced(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    pDevObj3->SetTextureStageStateForced(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    pDevObj3->SetTextureStageStateForced(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    pDevObj3->SetTextureStageStateForced(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    // Restore TEXTUREFACTOR to white (neutral)
    _pDev->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFFFFFFFF);

    // Restore original View/Proj matrices
    g_Render.SetTransformView(&_matSavedView);
    g_Render.SetTransformProj(&_matSavedProj);
}

void CMPShadowMap::BindShadowMap(int textureStage) {
    if (!IsEnabled() || !_pShadowTexture)
        return;

    _pDev->SetTexture(textureStage, _pShadowTexture);

    // Set appropriate sampler state for shadow map lookups
    // LINEAR allows hardware bilinear blending between PCF taps for smoother shadow edges
    _pDev->SetSamplerState(textureStage, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    _pDev->SetSamplerState(textureStage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    _pDev->SetSamplerState(textureStage, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    _pDev->SetSamplerState(textureStage, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
    _pDev->SetSamplerState(textureStage, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
    _pDev->SetSamplerState(textureStage, D3DSAMP_BORDERCOLOR, 0xFFFFFFFF);
}

void CMPShadowMap::UnbindShadowMap(int textureStage) {
    if (_pDev)
        _pDev->SetTexture(textureStage, nullptr);
}

void CMPShadowMap::BuildGroundGrid() {
    if (!_pDev)
        return;

    // Release old buffers
    if (_pGroundVB) { _pGroundVB->Release(); _pGroundVB = nullptr; }
    if (_pGroundIB) { _pGroundIB->Release(); _pGroundIB = nullptr; }

    // Simple quad: 4 vertices, 2 triangles
    // Eliminates grid seam artifacts that cause visible lines during movement
    _nGroundVertCount = 4;
    _nGroundTriCount = 2;

    struct GroundVert { float x, y, z; };
    HRESULT hr = _pDev->CreateVertexBuffer(
        4 * sizeof(GroundVert),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        D3DFVF_XYZ,
        D3DPOOL_DEFAULT,
        &_pGroundVB,
        nullptr
    );
    if (FAILED(hr)) {
        LG("shadow", "Failed to create ground VB: 0x%08X\n", hr);
        return;
    }

    hr = _pDev->CreateIndexBuffer(
        6 * sizeof(WORD),
        D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &_pGroundIB,
        nullptr
    );
    if (FAILED(hr)) {
        LG("shadow", "Failed to create ground IB: 0x%08X\n", hr);
        _pGroundVB->Release();
        _pGroundVB = nullptr;
        return;
    }

    // Fill index buffer (two triangles forming a quad)
    WORD* pIdx = nullptr;
    hr = _pGroundIB->Lock(0, 0, (void**)&pIdx, 0);
    if (SUCCEEDED(hr)) {
        pIdx[0] = 0; pIdx[1] = 1; pIdx[2] = 2;
        pIdx[3] = 2; pIdx[4] = 1; pIdx[5] = 3;
        _pGroundIB->Unlock();
    }
}

void CMPShadowMap::RenderGroundOverlay(const D3DXMATRIX& matViewProj) {
    if (!IsEnabled() || !_pShadowTexture || !_pShadowEffect || !_pGroundVB || !_pGroundIB)
        return;

    // Update the quad vertex positions centered on the smoothed focus point
    float halfSize = _config.orthoSize;
    float x0 = _vFocusPointSmoothed.x - halfSize;
    float y0 = _vFocusPointSmoothed.y - halfSize;
    float x1 = _vFocusPointSmoothed.x + halfSize;
    float y1 = _vFocusPointSmoothed.y + halfSize;
    float z  = _vFocusPointSmoothed.z;

    struct GroundVert { float x, y, z; };
    GroundVert* pVerts = nullptr;
    HRESULT hr = _pGroundVB->Lock(0, 0, (void**)&pVerts, D3DLOCK_DISCARD);
    if (FAILED(hr))
        return;

    pVerts[0].x = x0; pVerts[0].y = y0; pVerts[0].z = z;  // top-left
    pVerts[1].x = x1; pVerts[1].y = y0; pVerts[1].z = z;  // top-right
    pVerts[2].x = x0; pVerts[2].y = y1; pVerts[2].z = z;  // bottom-left
    pVerts[3].x = x1; pVerts[3].y = y1; pVerts[3].z = z;  // bottom-right
    _pGroundVB->Unlock();

    // Set the ShadowReceive technique
    _pShadowEffect->SetTechnique("ShadowReceive");

    // Set shader parameters
    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    _pShadowEffect->SetMatrix("g_matWorld", &matIdentity);
    _pShadowEffect->SetMatrix("g_matWorldViewProj", &matViewProj);
    _pShadowEffect->SetMatrix("g_matShadowTransform", &_matShadowTransform);
    _pShadowEffect->SetFloat("g_fShadowIntensity", _config.shadowIntensity);
    _pShadowEffect->SetFloat("g_fDepthBias", _config.depthBias);
    _pShadowEffect->SetFloat("g_fTexelSize", 1.0f / (float)_config.resolution);
    _pShadowEffect->SetFloat("g_fBorderFade", 0.10f);  // 10% fade zone at each projection edge
    _pShadowEffect->SetFloat("g_fPCFRadius",  5.0f);   // Poisson kernel spread in texels
    _pShadowEffect->SetTexture("g_texShadowMap", _pShadowTexture);

    // Disable Z-test so the overlay renders on top of already-drawn terrain
    // (we render after terrain but before scene objects/characters, so
    // those will correctly occlude the shadow overlay with their own Z-writes)
    DWORD oldZEnable;
    _pDev->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
    _pDev->SetRenderState(D3DRS_ZENABLE, FALSE);

    // Render the ground overlay
    UINT nPasses = 0;
    _pShadowEffect->Begin(&nPasses, 0);
    if (nPasses > 0) {
        _pShadowEffect->BeginPass(0);

        _pDev->SetStreamSource(0, _pGroundVB, 0, sizeof(GroundVert));
        _pDev->SetIndices(_pGroundIB);
        _pDev->SetFVF(D3DFVF_XYZ);
        _pDev->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            0, 0,
            _nGroundVertCount,
            0,
            _nGroundTriCount
        );

        _pShadowEffect->EndPass();
    }
    _pShadowEffect->End();

    // Restore Z-test
    _pDev->SetRenderState(D3DRS_ZENABLE, oldZEnable);
}

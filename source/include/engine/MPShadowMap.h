#pragma once

#include "MindPowerAPI.h"
#include "lwDirectX.h"
#include <d3dx9.h>

// Shadow map resolution presets
enum eShadowMapQuality {
    SHADOW_QUALITY_LOW    = 512,
    SHADOW_QUALITY_MEDIUM = 1024,
    SHADOW_QUALITY_HIGH   = 2048,
};

// Shadow map configuration
struct ShadowMapConfig {
    int   resolution;         // Texture resolution (512, 1024, 2048)
    float orthoSize;          // Orthographic projection half-size (world units)
    float nearPlane;          // Light frustum near plane
    float farPlane;           // Light frustum far plane
    float depthBias;          // Depth bias to reduce shadow acne
    float slopeScaleBias;     // Slope-scale depth bias
    float shadowIntensity;    // Shadow darkness (0=invisible, 1=fully dark)
    float lightHeight;        // Height of the light source above the focus point
    bool  enabled;            // Master toggle

    ShadowMapConfig()
        : resolution(SHADOW_QUALITY_HIGH)
        , orthoSize(30.0f)
        , nearPlane(1.0f)
        , farPlane(200.0f)
        , depthBias(0.0005f)
        , slopeScaleBias(1.0f)
        , shadowIntensity(0.55f)
        , lightHeight(100.0f)
        , enabled(true)
    {}
};

class MINDPOWER_API CMPShadowMap {
public:
    CMPShadowMap();
    ~CMPShadowMap();

    // Initialization and cleanup
    bool Create(IDirect3DDeviceX* pDev, const ShadowMapConfig& config);
    void Release();

    // Device lost/reset (for fullscreen toggle, etc.)
    void OnLostDevice();
    bool OnResetDevice();

    // Configuration
    void SetConfig(const ShadowMapConfig& config);
    const ShadowMapConfig& GetConfig() const { return _config; }
    void SetEnabled(bool enabled) { _config.enabled = enabled; }
    bool IsEnabled() const { return _config.enabled && _bInitialized; }
    float GetOrthoSize() const { return _config.orthoSize; }

    // Light direction (normalized, pointing FROM light TO scene)
    void SetLightDirection(const D3DXVECTOR3& dir);
    void SetLightDirection(float x, float y, float z);
    const D3DXVECTOR3& GetLightDirection() const { return _vLightDir; }

    // Focus point (where the shadow map is centered, typically the camera target)
    void SetFocusPoint(const D3DXVECTOR3& pos);
    const D3DXVECTOR3& GetFocusPoint() const { return _vFocusPoint; }

    // Shadow pass - call these around rendering shadow casters
    bool BeginShadowPass();
    void EndShadowPass();
    void SetAlphaCutoutCasterMode(bool enabled);

    // Bind shadow map for reading during the main scene pass
    // Binds the shadow texture to the specified texture stage
    // and sets the light view-projection matrix as shader constants
    void BindShadowMap(int textureStage = 3);
    void UnbindShadowMap(int textureStage = 3);

    // Render a ground-aligned overlay quad that applies shadow darkening
    // onto already-rendered terrain. Call AFTER terrain renders.
    // Requires the camera's world-view-projection matrix for the overlay vertices.
    void RenderGroundOverlay(const D3DXMATRIX& matViewProj);

    // Access the light-space matrices
    const D3DXMATRIX& GetLightViewMatrix() const { return _matLightView; }
    const D3DXMATRIX& GetLightProjMatrix() const { return _matLightProj; }
    const D3DXMATRIX& GetLightViewProjMatrix() const { return _matLightViewProj; }
    const D3DXMATRIX& GetShadowTransformMatrix() const { return _matShadowTransform; }

    // Access shadow map texture (for custom shaders)
    IDirect3DTextureX* GetShadowTexture() const { return _pShadowTexture; }

    // Effect file for shadow rendering
    LPD3DXEFFECT GetShadowEffect() const { return _pShadowEffect; }

private:
    void UpdateLightMatrices();
    bool CreateResources();
    void ReleaseResources();
    void BuildGroundGrid();

private:
    IDirect3DDeviceX*      _pDev;
    ShadowMapConfig        _config;
    bool                   _bInitialized;

    // Shadow map render target
    IDirect3DTextureX*     _pShadowTexture;     // Shadow map texture (also used as render target)
    IDirect3DSurfaceX*     _pShadowSurface;     // Render target surface from the texture
    IDirect3DSurfaceX*     _pShadowDepthSurface; // Depth/stencil surface for shadow pass

    // Saved state for restore after shadow pass
    IDirect3DSurfaceX*     _pOldRenderTarget;
    IDirect3DSurfaceX*     _pOldDepthSurface;
    D3DVIEWPORTX           _oldViewport;
    D3DXMATRIX             _matSavedView;
    D3DXMATRIX             _matSavedProj;
    DWORD                  _savedAlphaRef;

    // Smoothed focus point — lerps toward _vFocusPoint each frame so
    // shadow position transitions (e.g. height changes) are gradual.
    D3DXVECTOR3            _vFocusPointSmoothed;
    bool                   _bFocusInitialized;  // Skip lerp on very first frame

    // Light-space matrices
    D3DXVECTOR3            _vLightDir;
    D3DXVECTOR3            _vFocusPoint;
    D3DXMATRIX             _matLightView;
    D3DXMATRIX             _matLightProj;
    D3DXMATRIX             _matLightViewProj;
    D3DXMATRIX             _matShadowTransform;  // ViewProj * texScale (for UV lookup)

    // Shadow effect (depth rendering + sampling)
    LPD3DXEFFECT           _pShadowEffect;

    // Ground overlay grid for terrain shadow rendering
    IDirect3DVertexBuffer9* _pGroundVB;
    IDirect3DIndexBuffer9*  _pGroundIB;
    int                    _nGroundGridSize;    // Grid subdivisions
    int                    _nGroundVertCount;
    int                    _nGroundTriCount;
};

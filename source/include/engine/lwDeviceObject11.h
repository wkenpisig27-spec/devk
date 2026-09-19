#pragma once

#include "MindPowerAPI.h"
#include "lwHeader.h"
#include "lwDirectX.h"
#include "lwD3DSettings.h"
#include "lwMath.h"
#include "lwClassDecl.h"
#include "lwInterfaceExt.h"
#include "lwPreDefinition.h"
#include "lwD3D11NativeContext.h"

LW_BEGIN

class lwSysGraphics;
class lwDeviceObject11;

MINDPOWER_API lwDeviceObject11* lwGetActiveDeviceObject11();

class lwDeviceObject11 : public lwIDeviceObject
{
LW_STD_DECLARATION()

public:
    explicit lwDeviceObject11(lwSysGraphics* sys_graphics);
    ~lwDeviceObject11();

    LW_RESULT CreateDirect3D();
    LW_RESULT CreateDevice(lwD3DCreateParam* param);
    LW_RESULT ResetDevice(D3DPRESENT_PARAMETERS* d3dpp);
    LW_RESULT ResetDeviceStateCache();
    LW_RESULT ResetDeviceTransformMatrix();

    lwD3DCreateParam* GetD3DCreateParam() { return &_d3d_create_param; }
    D3DDISPLAYMODE* GetAdapterDisplayMode() { return &_display_mode; }
    D3DCAPSX* GetDeviceCaps() { return &_dev_caps; }
    lwBackBufferFormatsInfo* GetBackBufferFormatsCaps() { return &_bbf_caps; }

    LW_RESULT SetStreamSource(UINT stream_num, IDirect3DVertexBufferX* stream_data, UINT offset_byte, UINT stride);
    LW_RESULT SetIndices(IDirect3DIndexBufferX* index_data, UINT base_vert_index);
    LW_RESULT DrawPrimitive(D3DPRIMITIVETYPE pt_type, UINT start_vertex, UINT count);
    LW_RESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE pt_type, INT base_vert_index, UINT min_index, UINT vert_num, UINT start_index, UINT count);
    LW_RESULT DrawPrimitiveUP(D3DPRIMITIVETYPE pt_type, UINT count, const void* data, UINT stride);
    LW_RESULT DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE pt_type, UINT min_vert_index, UINT vert_num, UINT count, const void* index_data, D3DFORMAT index_data_fmt, const void* vert_data, UINT vert_stride);

    LW_RESULT SetTransform(D3DTRANSFORMSTATETYPE state, const lwMatrix44* mat);
    LW_RESULT SetTransformView(const lwMatrix44* mat);
    LW_RESULT SetTransformProj(const lwMatrix44* mat);
    LW_RESULT SetTransformWorld(const lwMatrix44* mat);

    const lwMatrix44* GetMatProj() { return &_mat_proj; }
    const lwMatrix44* GetMatView() { return &_mat_view; }
    const lwMatrix44* GetMatViewProj() { return &_mat_viewproj; }

    LW_RESULT SetMaterial(lwMaterial* mtl);

    LW_RESULT SetTexture(DWORD stage, IDirect3DBaseTextureX* tex);
    LW_RESULT SetRenderState(D3DRENDERSTATETYPE state, DWORD value);
    LW_RESULT SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value);
    LW_RESULT SetTextureForced(DWORD stage, IDirect3DTextureX* tex);
    LW_RESULT SetRenderStateForced(D3DRENDERSTATETYPE state, DWORD value);
    LW_RESULT SetTextureStageStateForced(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value);
    LW_RESULT SetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value);
    LW_RESULT SetSamplerStateForced(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value);

    LW_RESULT SetFVF(DWORD fvf);
    LW_RESULT SetVertexShader(IDirect3DVertexShaderX* shader);
    LW_RESULT SetVertexShaderForced(IDirect3DVertexShaderX* shader);
    LW_RESULT SetVertexDeclaration(IDirect3DVertexDeclarationX* decl);
    LW_RESULT SetVertexDeclarationForced(IDirect3DVertexDeclarationX* decl);
    LW_RESULT SetVertexShaderConstantF(UINT reg_id, const float* data, UINT v_num);

    LW_RESULT GetRenderState(DWORD state, DWORD* value);
    LW_RESULT GetTextureStageState(DWORD stage, DWORD state, DWORD* value);
    LW_RESULT GetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE state, DWORD* value);
    LW_RESULT GetTexture(DWORD stage, IDirect3DBaseTextureX** tex);

    LW_RESULT SetLight(DWORD id, const D3DLIGHTX* light);
    LW_RESULT GetLight(DWORD id, D3DLIGHTX* light);
    LW_RESULT LightEnable(DWORD id, BOOL flag);
    LW_RESULT LightEnableForced(DWORD id, BOOL flag);
    LW_RESULT GetLightEnable(DWORD id, BOOL* flag);
    LW_RESULT UpdateWindowRect();
    LW_RESULT GetWindowRect(RECT* rc_wnd, RECT* rc_client);

    LW_RESULT InitStateCache();
    LW_RESULT InitCapsInfo();

    LW_RESULT CreateVertexBuffer(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool, IDirect3DVertexBufferX** vb, HANDLE* handle);
    LW_RESULT CreateIndexBuffer(UINT length, DWORD usage, D3DFORMAT fmt, D3DPOOL pool, IDirect3DIndexBufferX** ib, HANDLE* handle);
    LW_RESULT CreateTexture(IDirect3DTextureX** out_tex, const lwTexDataInfo* info, DWORD level, DWORD usage, DWORD format, D3DPOOL pool);
    LW_RESULT CreateTexture(IDirect3DTextureX** out_tex, UINT width, UINT height, UINT level, DWORD usage, D3DFORMAT format, D3DPOOL pool);
    LW_RESULT CreateTextureFromFileInMemory(IDirect3DTextureX** out_tex, void* data, UINT data_size, UINT width, UINT height, UINT mip_level, DWORD usage, D3DFORMAT format, D3DPOOL pool, DWORD filter, DWORD mip_filter, D3DCOLOR colorkey, D3DXIMAGE_INFO* src_info, PALETTEENTRY* palette);
    LW_RESULT CreateVertexBuffer(lwIVertexBuffer** out_obj);
    LW_RESULT CreateIndexBuffer(lwIIndexBuffer** out_obj);
    LW_RESULT CreateRenderTarget(IDirect3DSurfaceX** o_surface, UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multi_sample, DWORD multi_sample_quality, BOOL lockable, HANDLE* handle);
    LW_RESULT CreateDepthStencilSurface(IDirect3DSurfaceX** o_surface, UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multi_sample, DWORD multi_sample_quality, BOOL discard, HANDLE* handle);
    LW_RESULT CreateCubeTexture(IDirect3DCubeTextureX** o_tex, UINT edge_length, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, HANDLE* handle);
    LW_RESULT CreateOffscreenPlainSurface(IDirect3DSurfaceX** surface, UINT width, UINT height, D3DFORMAT format, DWORD pool, HANDLE* handle);
    LW_RESULT ReleaseTex(IDirect3DTextureX* tex);
    LW_RESULT ReleaseVertexBuffer(IDirect3DVertexBufferX* vb);
    LW_RESULT ReleaseIndexBuffer(IDirect3DIndexBufferX* ib);

    void BeginBenchMark();
    void EndBenchMark();
    DWORD GetMarkPolygonNum() const { return _mark_polygon_num; }
    lwWatchDevVideoMemInfo* GetWatchVideoMemInfo() { return &_watch_vm_info; }

    LW_RESULT CheckCurrentDeviceFormat(DWORD type, D3DFORMAT check_fmt);
    LW_RESULT ScreenToWorld(lwVector3* org, lwVector3* ray, int x, int y);
    LW_RESULT WorldToScreen(int* x, int* y, float* z, const lwVector3* v);
    LW_RESULT DumpRenderState(const char* file);

    LW_RESULT SetViewPort(const D3DVIEWPORTX* vp);
    LW_RESULT GetViewPort(D3DVIEWPORTX* vp);

    LW_RESULT Clear(DWORD flags, D3DCOLOR color, float z, DWORD stencil);
    LW_RESULT Present();
    LW_RESULT BeginScene();
    LW_RESULT EndScene();
    LW_RESULT ResolveScenePost();

    void SetShadowPassMode(bool enabled) { _bShadowPass = enabled; }
    bool IsShadowPassMode() const { return _bShadowPass; }
    void PushOffscreenTargets(ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv);
    void PopOffscreenTargets();
    void UnbindPixelTextures();

    ID3D11Device* GetD3D11Device()
    {
        ID3D11Device* n = lwD3D11NativeGetDevice();
        return n ? n : _device;
    }
    ID3D11DeviceContext* GetD3D11Context()
    {
        ID3D11DeviceContext* n = lwD3D11NativeGetContext();
        return n ? n : _context;
    }
    IDXGISwapChain* GetSwapChain()
    {
        IDXGISwapChain* n = lwD3D11NativeGetSwapChain();
        return n ? n : _swapchain;
    }

    IDirect3DVertexBufferX* GetBoundVB() const { return _bound_vb; }
    UINT GetBoundVBOffset() const { return _bound_vb_off; }
    UINT GetBoundVBStride() const { return _bound_vb_stride; }
    IDirect3DIndexBufferX* GetBoundIB() const { return _bound_ib; }
    DWORD GetBoundFVF() const { return _bound_fvf; }
    IDirect3DVertexShaderX* GetBoundVS() const { return _bound_vs; }
    IDirect3DVertexDeclarationX* GetBoundDecl() const { return _bound_decl; }
    const float* GetVSConstants() const { return _vs_c; }
    const lwMatrix44* GetMatWorld() const { return &_mat_world; }
    const lwMatrix44* GetMatTex(DWORD stage) const
    {
        return (stage < 8) ? &_mat_tex[stage] : 0;
    }
    DWORD GetCachedTSS(DWORD stage, DWORD type) const
    {
        return (stage < LW_MAX_TEXTURESTAGE_NUM && type < LW_MAX_TEXTURESTAGESTATE_NUM)
            ? _tss_value[stage][type] : 0;
    }
    IDirect3DBaseTextureX* GetBoundTex(DWORD stage) const
    {
        return (stage < LW_MAX_TEXTURESTAGE_NUM) ? _tex_seq[stage] : 0;
    }
    const lwMaterial* GetBoundMaterial() const { return &_material; }
    const D3DLIGHTX* GetBoundLight(DWORD id) const
    {
        return (id < LW_MAX_LIGHT_NUM) ? &_light_seq[id] : 0;
    }
    BOOL GetBoundLightEnable(DWORD id) const
    {
        return (id < LW_MAX_LIGHT_NUM) ? _light_enable[id] : FALSE;
    }
    DWORD GetCachedRS(DWORD state) const
    {
        return (state < LW_MAX_RENDERSTATE_NUM) ? _rs_value[state] : 0;
    }
    DWORD GetCachedSS(DWORD sampler, DWORD type) const
    {
        return (sampler < LW_MAX_SAMPLESTAGE_NUM && type < LW_MAX_SAMPLESTATE_NUM)
            ? _ss_value[sampler][type] : 0;
    }

private:
    void _ReleaseTargets();
    LW_RESULT _CreateTargets();
    void _BindTargets();
    void _FillSyntheticCaps();

    lwSysGraphics* _sys_graphics;
    lwD3DCreateParam _d3d_create_param;
    D3DDISPLAYMODE _display_mode;
    D3DCAPSX _dev_caps;
    D3DVIEWPORTX _viewport;
    lwBackBufferFormatsInfo _bbf_caps;
    RECT _rc_window;
    RECT _rc_client;

    lwMatrix44 _mat_view;
    lwMatrix44 _mat_proj;
    lwMatrix44 _mat_viewproj;
    lwMatrix44 _mat_world;
    lwMatrix44 _mat_tex[8];
    lwMaterial _material;

    DWORD _rs_value[LW_MAX_RENDERSTATE_NUM];
    DWORD _tss_value[LW_MAX_TEXTURESTAGE_NUM][LW_MAX_TEXTURESTAGESTATE_NUM];
    DWORD _ss_value[LW_MAX_SAMPLESTAGE_NUM][LW_MAX_SAMPLESTATE_NUM];
    IDirect3DBaseTextureX* _tex_seq[LW_MAX_TEXTURESTAGE_NUM];
    BOOL _light_enable[LW_MAX_LIGHT_NUM];
    D3DLIGHTX _light_seq[LW_MAX_LIGHT_NUM];

    lwWatchDevVideoMemInfo _watch_vm_info;
    DWORD _mark_polygon_num;

    ID3D11Device* _device;
    ID3D11DeviceContext* _context;
    IDXGIFactory* _factory;
    IDXGISwapChain* _swapchain;
    ID3D11Texture2D* _depth_tex;
    ID3D11RenderTargetView* _bb_rtv;
    ID3D11DepthStencilView* _bb_dsv;
    ID3D11RenderTargetView* _rtv;
    ID3D11DepthStencilView* _dsv;
    ID3D11RenderTargetView* _saved_rtv;
    ID3D11DepthStencilView* _saved_dsv;
    int _offscreen_push;
    int _bShadowPass;
    UINT _bb_width;
    UINT _bb_height;
    UINT _msaa_count;
    UINT _msaa_quality;
    UINT _bb_msaa;
    int _vsync;
    int _post_resolved;

    IDirect3DVertexBufferX* _bound_vb;
    UINT _bound_vb_off;
    UINT _bound_vb_stride;
    IDirect3DIndexBufferX* _bound_ib;
    DWORD _bound_fvf;
    IDirect3DVertexShaderX* _bound_vs;
    IDirect3DVertexDeclarationX* _bound_decl;
    float _vs_c[256 * 4];
    IDirect3DVertexBufferX* _up_vb;
    UINT _up_vb_bytes;
};

LW_END
